/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/bluetooth/cs.h>

void estimate_distance(uint8_t *local_steps, uint16_t local_steps_len, uint8_t *peer_steps,
		       uint16_t peer_steps_len, uint8_t n_ap, enum bt_conn_le_cs_role role);
