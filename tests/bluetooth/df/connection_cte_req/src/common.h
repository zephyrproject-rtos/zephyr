/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

uint16_t common_create_connection(void);
void common_destroy_connection(uint16_t handle);
void common_set_peer_features(uint16_t conn_handle, uint64_t features);
void common_set_periph_latency(uint16_t conn_handle, uint16_t periph_latency);
