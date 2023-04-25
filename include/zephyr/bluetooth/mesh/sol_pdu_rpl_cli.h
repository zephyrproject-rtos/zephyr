/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @defgroup bt_mesh_sol_pdu_rpl_cli Bluetooth Mesh Solicitation PDU RPL Client
 * @{
 * @brief
 */

#ifndef BT_MESH_SOL_PDU_RPL_CLI_H__
#define BT_MESH_SOL_PDU_RPL_CLI_H__

#include <zephyr/bluetooth/mesh.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Solicitation PDU RPL Client Model Context */
struct bt_mesh_sol_pdu_rpl_cli {
	/** Solicitation PDU RPL model entry pointer. */
	const struct bt_mesh_model *model;

	/* Internal parameters for tracking message responses. */
	struct bt_mesh_msg_ack_ctx ack_ctx;

	/** @brief Optional callback for Solicitation PDU RPL Status messages.
	 *
	 *  Handles received Solicitation PDU RPL Status messages from a Solicitation
	 *  PDU RPL server.The @c start param represents the start of range that server
	 *  has cleared. The @c length param represents length of range cleared by server.
	 *
	 *  @param cli         Solicitation PDU RPL client that received the status message.
	 *  @param addr        Address of the sender.
	 *  @param range_start       Range start value.
	 *  @param range_length      Range length value.
	 */
	void (*srpl_status)(struct bt_mesh_sol_pdu_rpl_cli *cli, uint16_t addr,
			    uint16_t range_start, uint8_t range_length);
};

/**
 *  @brief Solicitation PDU RPL Client model composition data entry.
 */
#define BT_MESH_MODEL_SOL_PDU_RPL_CLI(cli_data)                             \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_SOL_PDU_RPL_CLI,                  \
			 _bt_mesh_sol_pdu_rpl_cli_op, NULL, cli_data,       \
			 &_bt_mesh_sol_pdu_rpl_cli_cb)

/** @brief Remove entries from Solicitation PDU RPL of addresses in given range.
 *
 *  This method can be used asynchronously by setting @p start_rsp or
 *  @p len_rsp as NULL. This way the method will not wait for response and will
 *  return immediately after sending the command.
 *
 *  To process the response arguments of an async method, register
 *  the @c srpl_status callback in @c bt_mesh_sol_pdu_rpl_cli struct.
 *
 *  @param ctx           Message context for the message.
 *  @param range_start   Start of Unicast address range.
 *  @param range_len     Length of Unicast address range. Valid values are 0x00 and 0x02
 *                       to 0xff.
 *  @param start_rsp     Range start response buffer.
 *  @param len_rsp       Range length response buffer.
 *
 *  @return 0 on success, or (negative) error code otherwise.
 */
int bt_mesh_sol_pdu_rpl_clear(struct bt_mesh_msg_ctx *ctx, uint16_t range_start,
			      uint8_t range_len, uint16_t *start_rsp, uint8_t *len_rsp);


/** @brief Remove entries from Solicitation PDU RPL of addresses in given range (unacked).
 *
 *  @param ctx           Message context for the message.
 *  @param range_start   Start of Unicast address range.
 *  @param range_len     Length of Unicast address range. Valid values are 0x00 and 0x02
 *                       to 0xff.
 *
 *  @return 0 on success, or (negative) error code otherwise.
 */
int bt_mesh_sol_pdu_rpl_clear_unack(struct bt_mesh_msg_ctx *ctx, uint16_t range_start,
				    uint8_t range_len);

/** @brief Set the transmission timeout value.
 *
 *  @param timeout The new transmission timeout in milliseconds.
 */
void bt_mesh_sol_pdu_rpl_cli_timeout_set(int32_t timeout);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_sol_pdu_rpl_cli_op[];
extern const struct bt_mesh_model_cb _bt_mesh_sol_pdu_rpl_cli_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_SOL_PDU_RPL_CLI_H__ */

/** @} */
