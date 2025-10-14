/*
 * Copyright (c) 2025 Xiaomi Corporation
 * Copyright (c) 2017-2021 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int bt_br_init(void);

void bt_br_discovery_reset(void);

bool bt_br_update_sec_level(struct bt_conn *conn);

int bt_br_write_local_name(const char *name);
