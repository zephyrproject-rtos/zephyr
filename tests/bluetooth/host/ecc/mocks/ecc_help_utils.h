/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/slist.h>

typedef void (*bt_dh_key_cb_t)(const uint8_t *key);

/* ecc.c declarations */
uint8_t const *bt_ecc_get_public_key(void);
uint8_t const *bt_ecc_get_internal_debug_public_key(void);
sys_slist_t *bt_ecc_get_pub_key_cb_slist(void);
bt_dh_key_cb_t *bt_ecc_get_dh_key_cb(void);
