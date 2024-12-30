/** @file
 *  @brief Health Client Model APIs.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_MESH_HEALTH_CLI_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_MESH_HEALTH_CLI_H_

#include <zephyr/bluetooth/mesh.h>

/**
 * @brief Health Client Model
 * @defgroup bt_mesh_health_cli Health Client Model
 * @ingroup bt_mesh
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Health Client Model Context */
struct bt_mesh_health_cli {
	/** Composition data model entry pointer. */
	const struct bt_mesh_model *model;

	/** Publication structure instance */
	struct bt_mesh_model_pub pub;

	/** Publication buffer */
	struct net_buf_simple pub_buf;

	/** Publication data */
	uint8_t pub_data[BT_MESH_MODEL_BUF_LEN(BT_MESH_MODEL_OP_2(0x80, 0x32), 3)];

	/** @brief Optional callback for Health Period Status messages.
	 *
	 *  Handles received Health Period Status messages from a Health
	 *  server. The @c divisor param represents the period divisor value.
	 *
	 *  @param cli         Health client that received the status message.
	 *  @param addr        Address of the sender.
	 *  @param divisor     Health Period Divisor value.
	 */
	void (*period_status)(struct bt_mesh_health_cli *cli, uint16_t addr,
			      uint8_t divisor);

	/** @brief Optional callback for Health Attention Status messages.
	 *
	 *  Handles received Health Attention Status messages from a Health
	 *  server. The @c attention param represents the current attention value.
	 *
	 *  @param cli         Health client that received the status message.
	 *  @param addr        Address of the sender.
	 *  @param attention   Current attention value.
	 */
	void (*attention_status)(struct bt_mesh_health_cli *cli, uint16_t addr,
				 uint8_t attention);

	/** @brief Optional callback for Health Fault Status messages.
	 *
	 *  Handles received Health Fault Status messages from a Health
	 *  server. The @c fault array represents all faults that are
	 *  currently present in the server's element.
	 *
	 *  @see bt_mesh_health_faults
	 *
	 *  @param cli         Health client that received the status message.
	 *  @param addr        Address of the sender.
	 *  @param test_id     Identifier of a most recently performed test.
	 *  @param cid         Company Identifier of the node.
	 *  @param faults      Array of faults.
	 *  @param fault_count Number of faults in the fault array.
	 */
	void (*fault_status)(struct bt_mesh_health_cli *cli, uint16_t addr,
			     uint8_t test_id, uint16_t cid, uint8_t *faults,
			     size_t fault_count);

	/** @brief Optional callback for Health Current Status messages.
	 *
	 *  Handles received Health Current Status messages from a Health
	 *  server. The @c fault array represents all faults that are
	 *  currently present in the server's element.
	 *
	 *  @see bt_mesh_health_faults
	 *
	 *  @param cli         Health client that received the status message.
	 *  @param addr        Address of the sender.
	 *  @param test_id     Identifier of a most recently performed test.
	 *  @param cid         Company Identifier of the node.
	 *  @param faults      Array of faults.
	 *  @param fault_count Number of faults in the fault array.
	 */
	void (*current_status)(struct bt_mesh_health_cli *cli, uint16_t addr,
			       uint8_t test_id, uint16_t cid, uint8_t *faults,
			       size_t fault_count);

	/** @brief Optional callback for updating the message to be sent as periodic publication.
	 *
	 *  This callback is called before sending the periodic publication message.
	 *  The callback can be used to update the message to be sent.
	 *
	 *  If this callback is not implemented, periodic publication can still be enabled,
	 *  but no messages will be sent.
	 *
	 *  @param cli Health client that is sending the periodic publication message.
	 *  @param pub_buf Publication message buffer to be updated.
	 *
	 *  @return 0 if @p pub_buf is updated successfully, or (negative) error code on failure.
	 *            The message won't be sent if an error is returned.
	 */
	int (*update)(struct bt_mesh_health_cli *cli, struct net_buf_simple *pub_buf);

	/* Internal parameters for tracking message responses. */
	struct bt_mesh_msg_ack_ctx ack_ctx;
};

/**
 *  @brief Generic Health Client model composition data entry.
 *
 *  @param cli_data Pointer to a @ref bt_mesh_health_cli instance.
 */
#define BT_MESH_MODEL_HEALTH_CLI(cli_data)                                     \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_HEALTH_CLI, bt_mesh_health_cli_op,   \
			 &(cli_data)->pub, cli_data, &bt_mesh_health_cli_cb)

/** @brief Get the registered fault state for the given Company ID.
 *
 *  This method can be used asynchronously by setting @p test_id
 *  and ( @p faults or @p fault_count ) as NULL This way the method
 *  will not wait for response and will return immediately after
 *  sending the command.
 *
 *  To process the response arguments of an async method, register
 *  the @c fault_status callback in @c bt_mesh_health_cli struct.
 *
 *  @see bt_mesh_health_faults
 *
 *  @param cli Client model to send on.
 *  @param ctx Message context, or NULL to use the configured publish
 *  parameters.
 *  @param cid         Company ID to get the registered faults of.
 *  @param test_id     Test ID response buffer.
 *  @param faults      Fault array response buffer.
 *  @param fault_count Fault count response buffer.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_health_cli_fault_get(struct bt_mesh_health_cli *cli, struct bt_mesh_msg_ctx *ctx,
				 uint16_t cid, uint8_t *test_id, uint8_t *faults,
				 size_t *fault_count);

/** @brief Clear the registered faults for the given Company ID.
 *
 *  This method can be used asynchronously by setting @p test_id
 *  and ( @p faults or @p fault_count ) as NULL This way the method
 *  will not wait for response and will return immediately after
 *  sending the command.
 *
 *  To process the response arguments of an async method, register
 *  the @c fault_status callback in @c bt_mesh_health_cli struct.
 *
 *  @see bt_mesh_health_faults
 *
 *  @param cli Client model to send on.
 *  @param ctx Message context, or NULL to use the configured publish
 *  parameters.
 *  @param cid         Company ID to clear the registered faults for.
 *  @param test_id     Test ID response buffer.
 *  @param faults      Fault array response buffer.
 *  @param fault_count Fault count response buffer.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_health_cli_fault_clear(struct bt_mesh_health_cli *cli, struct bt_mesh_msg_ctx *ctx,
				   uint16_t cid, uint8_t *test_id, uint8_t *faults,
				   size_t *fault_count);

/** @brief Clear the registered faults for the given Company ID (unacked).
 *
 *  @see bt_mesh_health_faults
 *
 *  @param cli Client model to send on.
 *  @param ctx Message context, or NULL to use the configured publish
 *  parameters.
 *  @param cid         Company ID to clear the registered faults for.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_health_cli_fault_clear_unack(struct bt_mesh_health_cli *cli,
					 struct bt_mesh_msg_ctx *ctx, uint16_t cid);

/** @brief Invoke a self-test procedure for the given Company ID.
 *
 *  This method can be used asynchronously by setting @p faults
 *  or @p fault_count as NULL This way the method will not wait
 *  for response and will return immediately after sending the command.
 *
 *  To process the response arguments of an async method, register
 *  the @c fault_status callback in @c bt_mesh_health_cli struct.
 *
 *  @param cli Client model to send on.
 *  @param ctx Message context, or NULL to use the configured publish
 *  parameters.
 *  @param cid         Company ID to invoke the test for.
 *  @param test_id     Test ID response buffer.
 *  @param faults      Fault array response buffer.
 *  @param fault_count Fault count response buffer.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_health_cli_fault_test(struct bt_mesh_health_cli *cli, struct bt_mesh_msg_ctx *ctx,
				  uint16_t cid, uint8_t test_id, uint8_t *faults,
				  size_t *fault_count);

/** @brief Invoke a self-test procedure for the given Company ID (unacked).
 *
 *  @param cli Client model to send on.
 *  @param ctx Message context, or NULL to use the configured publish
 *  parameters.
 *  @param cid         Company ID to invoke the test for.
 *  @param test_id     Test ID response buffer.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_health_cli_fault_test_unack(struct bt_mesh_health_cli *cli, struct bt_mesh_msg_ctx *ctx,
					uint16_t cid, uint8_t test_id);

/** @brief Get the target node's Health fast period divisor.
 *
 *  The health period divisor is used to increase the publish rate when a fault
 *  is registered. Normally, the Health server will publish with the period in
 *  the configured publish parameters. When a fault is registered, the publish
 *  period is divided by (1 << divisor). For example, if the target node's
 *  Health server is configured to publish with a period of 16 seconds, and the
 *  Health fast period divisor is 5, the Health server will publish with an
 *  interval of 500 ms when a fault is registered.
 *
 *  This method can be used asynchronously by setting @p divisor
 *  as NULL. This way the method will not wait for response and will
 *  return immediately after sending the command.
 *
 *  To process the response arguments of an async method, register
 *  the @c period_status callback in @c bt_mesh_health_cli struct.
 *
 *  @param cli Client model to send on.
 *  @param ctx Message context, or NULL to use the configured publish
 *  parameters.
 *  @param divisor Health period divisor response buffer.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_health_cli_period_get(struct bt_mesh_health_cli *cli, struct bt_mesh_msg_ctx *ctx,
				  uint8_t *divisor);

/** @brief Set the target node's Health fast period divisor.
 *
 *  The health period divisor is used to increase the publish rate when a fault
 *  is registered. Normally, the Health server will publish with the period in
 *  the configured publish parameters. When a fault is registered, the publish
 *  period is divided by (1 << divisor). For example, if the target node's
 *  Health server is configured to publish with a period of 16 seconds, and the
 *  Health fast period divisor is 5, the Health server will publish with an
 *  interval of 500 ms when a fault is registered.
 *
 *  This method can be used asynchronously by setting @p updated_divisor
 *  as NULL. This way the method will not wait for response and will
 *  return immediately after sending the command.
 *
 *  To process the response arguments of an async method, register
 *  the @c period_status callback in @c bt_mesh_health_cli struct.
 *
 *  @param cli Client model to send on.
 *  @param ctx Message context, or NULL to use the configured publish
 *  parameters.
 *  @param divisor         New Health period divisor.
 *  @param updated_divisor Health period divisor response buffer.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_health_cli_period_set(struct bt_mesh_health_cli *cli, struct bt_mesh_msg_ctx *ctx,
				  uint8_t divisor, uint8_t *updated_divisor);

/** @brief Set the target node's Health fast period divisor (unacknowledged).
 *
 *  This is an unacknowledged version of this API.
 *
 *  @param cli Client model to send on.
 *  @param ctx Message context, or NULL to use the configured publish
 *  parameters.
 *  @param divisor         New Health period divisor.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_health_cli_period_set_unack(struct bt_mesh_health_cli *cli, struct bt_mesh_msg_ctx *ctx,
					uint8_t divisor);

/** @brief Get the current attention timer value.
 *
 *  This method can be used asynchronously by setting @p attention
 *  as NULL. This way the method will not wait for response and will
 *  return immediately after sending the command.
 *
 *  To process the response arguments of an async method, register
 *  the @c attention_status callback in @c bt_mesh_health_cli struct.
 *
 *  @param cli Client model to send on.
 *  @param ctx Message context, or NULL to use the configured publish
 *  parameters.
 *  @param attention Attention timer response buffer, measured in seconds.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_health_cli_attention_get(struct bt_mesh_health_cli *cli, struct bt_mesh_msg_ctx *ctx,
				     uint8_t *attention);

/** @brief Set the attention timer.
 *
 *  This method can be used asynchronously by setting @p updated_attention
 *  as NULL. This way the method will not wait for response and will
 *  return immediately after sending the command.
 *
 *  To process the response arguments of an async method, register
 *  the @c attention_status callback in @c bt_mesh_health_cli struct.
 *
 *  @param cli Client model to send on.
 *  @param ctx Message context, or NULL to use the configured publish
 *  parameters.
 *  @param attention         New attention timer time, in seconds.
 *  @param updated_attention Attention timer response buffer, measured in
 *                           seconds.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_health_cli_attention_set(struct bt_mesh_health_cli *cli, struct bt_mesh_msg_ctx *ctx,
				     uint8_t attention, uint8_t *updated_attention);

/** @brief Set the attention timer (unacknowledged).
 *
 *  @param cli Client model to send on.
 *  @param ctx Message context, or NULL to use the configured publish
 *  parameters.
 *  @param attention         New attention timer time, in seconds.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_health_cli_attention_set_unack(struct bt_mesh_health_cli *cli,
					   struct bt_mesh_msg_ctx *ctx, uint8_t attention);

/** @brief Get the current transmission timeout value.
 *
 *  @return The configured transmission timeout in milliseconds.
 */
int32_t bt_mesh_health_cli_timeout_get(void);

/** @brief Set the transmission timeout value.
 *
 *  @param timeout The new transmission timeout.
 */
void bt_mesh_health_cli_timeout_set(int32_t timeout);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op bt_mesh_health_cli_op[];
extern const struct bt_mesh_model_cb bt_mesh_health_cli_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_MESH_HEALTH_CLI_H_ */
