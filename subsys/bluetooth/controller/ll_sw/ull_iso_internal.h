/*
 * Copyright (c) 2020-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int ull_iso_init(void);
int ull_iso_reset(void);
void ull_iso_datapath_release(struct ll_iso_datapath *dp);
void ll_iso_rx_put(memq_link_t *link, void *rx);
void *ll_iso_rx_get(void);
void ll_iso_rx_dequeue(void);
void ll_iso_transmit_test_send_sdu(uint16_t handle, uint32_t ticks_at_expire);
