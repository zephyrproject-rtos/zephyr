/*
 * Copyright (c) 2020-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int ull_iso_init(void);
int ull_iso_reset(void);
void ull_iso_datapath_release(struct ll_iso_datapath *dp);
