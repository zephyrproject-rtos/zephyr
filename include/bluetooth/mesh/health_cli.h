/** @file
 *  @brief Bluetooth Mesh Health Client Model APIs.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_MESH_HEALTH_CLI_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_MESH_HEALTH_CLI_H_

/**
 * @brief Bluetooth Mesh
 * @defgroup bt_mesh_health_cli Bluetooth Mesh Health Client Model
 * @ingroup bt_mesh
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Mesh Health Client Model Context */
struct bt_mesh_health_cli {
	/** Composition data model entry pointer. */
	struct bt_mesh_model *model;

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

	/* Internal parameters for tracking message responses. */
	struct k_sem          op_sync;
	uint32_t                 op_pending;
	void                 *op_param;
};


/** @def BT_MESH_MODEL_HEALTH_CLI
 *
 *  @brief Generic Health Client model composition data entry.
 *
 *  @param cli_data Pointer to a @ref bt_mesh_health_cli instance.
 */
#define BT_MESH_MODEL_HEALTH_CLI(cli_data)                                     \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_HEALTH_CLI, bt_mesh_health_cli_op,   \
			 NULL, cli_data, &bt_mesh_health_cli_cb)

/** @brief Set Health client model instance to use for communication.
 *
 *  @param model Health Client model instance from the composition data.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_health_cli_set(struct bt_mesh_model *model);

/** @brief Get the registered fault state for the given Company ID.
 *
 *  @see bt_mesh_health_faults
 *
 *  @param addr        Target node element address.
 *  @param app_idx     Application index to encrypt with.
 *  @param cid         Company ID to get the registered faults of.
 *  @param test_id     Test ID response buffer.
 *  @param faults      Fault array response buffer.
 *  @param fault_count Fault count response buffer.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_health_fault_get(uint16_t addr, uint16_t app_idx, uint16_t cid,
				 uint8_t *test_id, uint8_t *faults,
				 size_t *fault_count);

/** @brief Clear the registered faults for the given Company ID.
 *
 *  @see bt_mesh_health_faults
 *
 *  @param addr        Target node element address.
 *  @param app_idx     Application index to encrypt with.
 *  @param cid         Company ID to clear the registered faults for.
 *  @param test_id     Test ID response buffer.
 *  @param faults      Fault array response buffer.
 *  @param fault_count Fault count response buffer.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_health_fault_clear(uint16_t addr, uint16_t app_idx, uint16_t cid,
				 uint8_t *test_id, uint8_t *faults,
				 size_t *fault_count);

/** @brief Invoke a self-test procedure for the given Company ID.
 *
 *  @param addr        Target node element address.
 *  @param app_idx     Application index to encrypt with.
 *  @param cid         Company ID to invoke the test for.
 *  @param test_id     Test ID response buffer.
 *  @param faults      Fault array response buffer.
 *  @param fault_count Fault count response buffer.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_health_fault_test(uint16_t addr, uint16_t app_idx, uint16_t cid,
				 uint8_t test_id, uint8_t *faults,
				 size_t *fault_count);

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
 *  @param addr    Target node element address.
 *  @param app_idx Application index to encrypt with.
 *  @param divisor Health period divisor response buffer.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_health_period_get(uint16_t addr, uint16_t app_idx, uint8_t *divisor);

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
 *  @param addr            Target node element address.
 *  @param app_idx         Application index to encrypt with.
 *  @param divisor         New Health period divisor.
 *  @param updated_divisor Health period divisor response buffer.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_health_period_set(uint16_t addr, uint16_t app_idx, uint8_t divisor,
				 uint8_t *updated_divisor);

/** @brief Get the current attention timer value.
 *
 *  @param addr      Target node element address.
 *  @param app_idx   Application index to encrypt with.
 *  @param attention Attention timer response buffer, measured in seconds.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_health_attention_get(uint16_t addr, uint16_t app_idx, uint8_t *attention);

/** @brief Set the attention timer.
 *
 *  @param addr              Target node element address.
 *  @param app_idx           Application index to encrypt with.
 *  @param attention         New attention timer time, in seconds.
 *  @param updated_attention Attention timer response buffer, measured in
 *                           seconds.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_health_attention_set(uint16_t addr, uint16_t app_idx, uint8_t attention,
				 uint8_t *updated_attention);

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
