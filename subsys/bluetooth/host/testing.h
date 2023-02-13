/**
 * @file testing.h
 * @brief Internal API for Bluetooth testing.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_MESH)
void bt_test_mesh_net_recv(uint8_t ttl, uint8_t ctl, uint16_t src, uint16_t dst,
			   const void *payload, size_t payload_len);
void bt_test_mesh_model_bound(uint16_t addr, struct bt_mesh_model *model,
			      uint16_t key_idx);
void bt_test_mesh_model_unbound(uint16_t addr, struct bt_mesh_model *model,
				uint16_t key_idx);
void bt_test_mesh_prov_invalid_bearer(uint8_t opcode);
void bt_test_mesh_trans_incomp_timer_exp(void);
#endif /* CONFIG_BT_MESH */
