/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @brief Send a model message.
 *
 * Sends a model message with the given context. If the message context is NULL, this
 * updates the publish message, and publishes with the configured publication parameters.
 *
 * @param model Model to send the message on.
 * @param ctx Message context, or NULL to send with the configured publish parameters.
 * @param buf Message to send.
 *
 * @retval 0 The message was sent successfully.
 * @retval -ENOTSUP A message context was not provided and publishing is not supported.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_msg_send(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		     struct net_buf_simple *buf);

/**
 * Message response context.
 */
struct bt_mesh_msg_rsp_ctx {
	struct bt_mesh_msg_ack_ctx *ack;       /**< Acknowledged message context. */
	uint32_t                    op;        /**< Opcode we're waiting for. */
	void                       *user_data; /**< User specific parameter. */
	int32_t                     timeout;   /**< Response timeout in milliseconds. */
};

/** @brief Send an acknowledged model message.
 *
 * Sends a model message with the given context. If the message context is NULL, this
 * updates the publish message, and publishes with the configured publication parameters.
 *
 * If a response context is provided, the call blocks for the time specified in
 * the response context, or until @ref bt_mesh_msg_ack_ctx_rx is called.
 *
 * @param model Model to send the message on.
 * @param ctx Message context, or NULL to send with the configured publish parameters.
 * @param buf Message to send.
 * @param rsp Message response context, or NULL if no response is expected.
 *
 * @retval 0 The message was sent successfully.
 * @retval -EBUSY A blocking request is already in progress.
 * @retval -ENOTSUP A message context was not provided and publishing is not supported.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_msg_ackd_send(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf, const struct bt_mesh_msg_rsp_ctx *rsp);
