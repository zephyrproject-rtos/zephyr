/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

uint16_t ut_bt_create_connection(void);
void ut_bt_destroy_connection(uint16_t handle);
void ut_bt_set_peer_features(uint16_t conn_handle, uint64_t features);
void ut_bt_set_periph_latency(uint16_t conn_handle, uint16_t periph_latency);
