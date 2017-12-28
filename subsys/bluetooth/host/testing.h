/**
 * @file testing.h
 * @brief Internal API for Bluetooth testing.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void bt_test_mesh_model_bound(u16_t addr, struct bt_mesh_model *model,
			      u16_t key_idx);
void bt_test_mesh_model_unbound(u16_t addr, struct bt_mesh_model *model,
				u16_t key_idx);
void bt_test_mesh_prov_invalid_bearer(u8_t opcode);
