/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int ull_tmp_enable(u16_t handle);
int ull_tmp_disable(u16_t handle);

int ull_tmp_data_send(u16_t handle, u8_t size, u8_t *data);
