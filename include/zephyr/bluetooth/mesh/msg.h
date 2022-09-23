/** @file
 *  @brief Message APIs.
 */

/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_MESH_MSG_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_MESH_MSG_H_

/**
 * @brief Message
 * @defgroup bt_mesh_msg Message
 * @ingroup bt_mesh
 * @{
 */

#include <zephyr/kernel.h>
#include <zephyr/net/buf.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Length of a short Mesh MIC. */
#define BT_MESH_MIC_SHORT 4
/** Length of a long Mesh MIC. */
#define BT_MESH_MIC_LONG 8

/**
 *  @brief Helper to determine the length of an opcode.
 *
 *  @param _op Opcode.
 */
#define BT_MESH_MODEL_OP_LEN(_op) ((_op) <= 0xff ? 1 : (_op) <= 0xffff ? 2 : 3)

/**
 *  @brief Helper for model message buffer length.
 *
 *  Returns the length of a Mesh model message buffer, including the opcode
 *  length and a short MIC.
 *
 *  @param _op          Opcode of the message.
 *  @param _payload_len Length of the model payload.
 */
#define BT_MESH_MODEL_BUF_LEN(_op, _payload_len)                               \
	(BT_MESH_MODEL_OP_LEN(_op) + (_payload_len) + BT_MESH_MIC_SHORT)

/**
 *  @brief Helper for model message buffer length.
 *
 *  Returns the length of a Mesh model message buffer, including the opcode
 *  length and a long MIC.
 *
 *  @param _op          Opcode of the message.
 *  @param _payload_len Length of the model payload.
 */
#define BT_MESH_MODEL_BUF_LEN_LONG_MIC(_op, _payload_len)                      \
	(BT_MESH_MODEL_OP_LEN(_op) + (_payload_len) + BT_MESH_MIC_LONG)

/**
 *  @brief Define a Mesh model message buffer using @ref NET_BUF_SIMPLE_DEFINE.
 *
 *  @param _buf         Buffer name.
 *  @param _op          Opcode of the message.
 *  @param _payload_len Length of the model message payload.
 */
#define BT_MESH_MODEL_BUF_DEFINE(_buf, _op, _payload_len)                      \
	NET_BUF_SIMPLE_DEFINE(_buf, BT_MESH_MODEL_BUF_LEN(_op, (_payload_len)))

/** Message sending context. */
struct bt_mesh_msg_ctx {
	/** NetKey Index of the subnet to send the message on. */
	uint16_t net_idx;

	/** AppKey Index to encrypt the message with. */
	uint16_t app_idx;

	/** Remote address. */
	uint16_t addr;

	/** Destination address of a received message. Not used for sending. */
	uint16_t recv_dst;

	/** RSSI of received packet. Not used for sending. */
	int8_t  recv_rssi;

	/** Received TTL value. Not used for sending. */
	uint8_t  recv_ttl;

	/** Force sending reliably by using segment acknowledgment */
	bool  send_rel;

	/** TTL, or BT_MESH_TTL_DEFAULT for default TTL. */
	uint8_t  send_ttl;
};

/** @brief Initialize a model message.
 *
 *  Clears the message buffer contents, and encodes the given opcode.
 *  The message buffer will be ready for filling in payload data.
 *
 *  @param msg    Message buffer.
 *  @param opcode Opcode to encode.
 */
void bt_mesh_model_msg_init(struct net_buf_simple *msg, uint32_t opcode);

/**
 * Acknowledged message context for tracking the status of model messages pending a response.
 */
struct bt_mesh_msg_ack_ctx {
	struct k_sem          sem;       /**< Sync semaphore. */
	uint32_t              op;        /**< Opcode we're waiting for. */
	uint16_t              dst;       /**< Address of the node that should respond. */
	void                 *user_data; /**< User specific parameter. */
};

/** @brief Initialize an acknowledged message context.
 *
 *  Initializes semaphore used for synchronization between @ref bt_mesh_msg_ack_ctx_wait and
 *  @ref bt_mesh_msg_ack_ctx_rx calls. Call this function before using @ref bt_mesh_msg_ack_ctx.
 *
 *  @param ack Acknowledged message context to initialize.
 */
static inline void bt_mesh_msg_ack_ctx_init(struct bt_mesh_msg_ack_ctx *ack)
{
	k_sem_init(&ack->sem, 0, 1);
}

/** @brief Reset the synchronization semaphore in an acknowledged message context.
 *
 *  This function aborts call to @ref bt_mesh_msg_ack_ctx_wait.
 *
 *  @param ack Acknowledged message context to be reset.
 */
static inline void bt_mesh_msg_ack_ctx_reset(struct bt_mesh_msg_ack_ctx *ack)
{
	k_sem_reset(&ack->sem);
}

/** @brief Clear parameters of an acknowledged message context.
 *
 *  This function clears the opcode, remote address and user data set
 *  by @ref bt_mesh_msg_ack_ctx_prepare.
 *
 *  @param ack Acknowledged message context to be cleared.
 */
void bt_mesh_msg_ack_ctx_clear(struct bt_mesh_msg_ack_ctx *ack);

/** @brief Prepare an acknowledged message context for the incoming message to wait.
 *
 *  This function sets the opcode, remote address of the incoming message and stores the user data.
 *  Use this function before calling @ref bt_mesh_msg_ack_ctx_wait.
 *
 *  @param ack       Acknowledged message context to prepare.
 *  @param op        The message OpCode.
 *  @param dst       Destination address of the message.
 *  @param user_data User data for the acknowledged message context.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_msg_ack_ctx_prepare(struct bt_mesh_msg_ack_ctx *ack,
				uint32_t op, uint16_t dst, void *user_data);

/** @brief Check if the acknowledged message context is initialized with an opcode.
 *
 *  @param ack Acknowledged message context.
 *
 *  @return true if the acknowledged message context is initialized with an opcode,
 *  false otherwise.
 */
static inline bool bt_mesh_msg_ack_ctx_busy(struct bt_mesh_msg_ack_ctx *ack)
{
	return (ack->op != 0);
}

/** @brief Wait for a message acknowledge.
 *
 *  This function blocks execution until @ref bt_mesh_msg_ack_ctx_rx is called or by timeout.
 *
 *  @param ack     Acknowledged message context of the message to wait for.
 *  @param timeout Wait timeout.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_msg_ack_ctx_wait(struct bt_mesh_msg_ack_ctx *ack, k_timeout_t timeout);

/** @brief Mark a message as acknowledged.
 *
 *  This function unblocks call to @ref bt_mesh_msg_ack_ctx_wait.
 *
 *  @param ack Context of a message to be acknowledged.
 */
static inline void bt_mesh_msg_ack_ctx_rx(struct bt_mesh_msg_ack_ctx *ack)
{
	k_sem_give(&ack->sem);
}

/** @brief Check if an opcode and address of a message matches the expected one.
 *
 *  @param ack       Acknowledged message context to be checked.
 *  @param op        OpCode of the incoming message.
 *  @param addr      Source address of the incoming message.
 *  @param user_data If not NULL, returns a user data stored in the acknowledged message context
 *  by @ref bt_mesh_msg_ack_ctx_prepare.
 *
 *  @return true if the incoming message matches the expected one, false otherwise.
 */
bool bt_mesh_msg_ack_ctx_match(const struct bt_mesh_msg_ack_ctx *ack,
			       uint32_t op, uint16_t addr, void **user_data);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_MESH_MSG_H_ */
