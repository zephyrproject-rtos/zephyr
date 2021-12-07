/*
 * Copyright (c) 2020-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int ull_iso_init(void);
int ull_iso_reset(void);
void ull_iso_datapath_release(struct ll_iso_datapath *dp);
uint8_t ull_iso_tx_ack_get(uint16_t *handle);
void ll_iso_rx_put(memq_link_t *link, void *rx);
void *ll_iso_rx_get(void);
void ll_iso_rx_dequeue(void);
