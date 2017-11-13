/** @file
 *  @brief Bluetooth Mesh Health Server Model APIs.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BT_MESH_HEALTH_SRV_H
#define __BT_MESH_HEALTH_SRV_H

/**
 * @brief Bluetooth Mesh Health Server Model
 * @defgroup bt_mesh_health_srv Bluetooth Mesh Health Server Model
 * @ingroup bt_mesh
 * @{
 */

/** Mesh Health Server Model Context */
struct bt_mesh_health_srv {
	struct bt_mesh_model *model;

	/* Fetch current faults */
	int (*fault_get_cur)(struct bt_mesh_model *model, u8_t *test_id,
			     u16_t *company_id, u8_t *faults,
			     u8_t *fault_count);

	/* Fetch registered faults */
	int (*fault_get_reg)(struct bt_mesh_model *model, u16_t company_id,
			     u8_t *test_id, u8_t *faults,
			     u8_t *fault_count);

	/* Clear registered faults */
	int (*fault_clear)(struct bt_mesh_model *model, u16_t company_id);

	/* Run a specific test */
	int (*fault_test)(struct bt_mesh_model *model, u8_t test_id,
			  u16_t company_id);

	/* Attention Timer state */
	struct {
		struct k_delayed_work timer;
		void (*on)(struct bt_mesh_model *model);
		void (*off)(struct bt_mesh_model *model);
	} attention;
};

int bt_mesh_fault_update(struct bt_mesh_elem *elem);

extern const struct bt_mesh_model_op bt_mesh_health_srv_op[];
extern struct bt_mesh_model_pub bt_mesh_health_pub;

#define BT_MESH_MODEL_HEALTH_SRV(srv_data)                                   \
		BT_MESH_MODEL(BT_MESH_MODEL_ID_HEALTH_SRV,                   \
			      bt_mesh_health_srv_op, &bt_mesh_health_pub,    \
			      srv_data)

/**
 * @}
 */

#endif /* __BT_MESH_HEALTH_SRV_H */
