/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct op_agg_ctx {
	/** Context is initialized. */
	bool initialized;

	/** NetKey Index of the subnet to send the message on. */
	uint16_t net_idx;

	/** AppKey Index to encrypt the message with. */
	uint16_t app_idx;

	/** Remote element address. */
	uint16_t addr;

	/** List of source element addresses.
	 * Used by Client to match aggregated responses
	 * with local source client models.
	 */
	struct net_buf_simple *srcs;

	/** Response error code. */
	int rsp_err;

	/** Aggregated message buffer. */
	struct net_buf_simple *sdu;

	/** Used only by the Opcodes Aggregator Server.
	 *
	 * Indicates that the received aggregated message
	 * was acknowledged by local server model.
	 */
	bool ack;
};

struct op_agg_ctx *bt_mesh_op_agg_ctx_get(void);
void bt_mesh_op_agg_ctx_reinit(void);

int bt_mesh_op_agg_encode_msg(struct net_buf_simple *msg);
int bt_mesh_op_agg_decode_msg(struct net_buf_simple *msg,
			      struct net_buf_simple *buf);

int bt_mesh_op_agg_accept(struct bt_mesh_msg_ctx *ctx);

int bt_mesh_op_agg_send(const struct bt_mesh_model *model,
			struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *msg,
			const struct bt_mesh_send_cb *cb);
