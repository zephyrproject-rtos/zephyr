/* rpa.h - Bluetooth Resolvable Private Addresses (RPA) generation and
 * resolution
 */

/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

int bt_rpa_create(const uint8_t irk[16], bt_addr_t *rpa);

/**
 * @brief Random address hash function ah, defined in Bluetooth Core Spec Vol 3, Part H, Section 2.2.2.
 *
 * @param irk Key of 16 octets, least significant octet first.
 * @param r   Random value of 3 octets, least significant octet first.
 * @param out Hash of 3 octets, least significant octet first.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_rpa_ah(const uint8_t irk[16], const uint8_t r[3], uint8_t out[3]);
