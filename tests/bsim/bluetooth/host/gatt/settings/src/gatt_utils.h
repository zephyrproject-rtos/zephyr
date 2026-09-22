/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>

void gatt_register_service_1(void);
void gatt_register_service_2(void);
void gatt_discover(void);
void activate_robust_caching(void);
void read_test_char(bool expect_success);
void wait_for_client_read(void);
void gatt_subscribe_to_service_changed(bool subscribe);
void wait_for_sc_indication(void);
void gatt_clear_flags(void);
