/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int ull_sync_init(void);
int ull_sync_reset(void);
uint16_t ull_sync_handle_get(struct ll_sync_set *sync);
void ull_sync_release(struct ll_sync_set *sync);
