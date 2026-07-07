/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BFLB_WIFI_IPC_H_
#define BFLB_WIFI_IPC_H_

#include <stdint.h>
#include "bflb_wifi.h"

/* Blob txdesc_host layout: { list_hdr 4 + host_id 4 + ready 4 +
 * pad_txdesc[208] + pad_buf[400] } padded to 624 bytes, two descriptors.
 */
#define BFLB_WIFI_TXDESC_STRIDE 624U
#define BFLB_WIFI_TXDESC_COUNT  2U

volatile uint8_t *bflb_wifi_ipc_txdesc(uint32_t idx);

int bflb_wifi_ipc_init(struct bflb_wifi_dev *d);
int bflb_wifi_ipc_send_cmd(struct bflb_wifi_dev *d, uint16_t msg_id, uint16_t cfm_id,
			   const void *params, uint32_t params_len, void *cfm_out,
			   uint32_t cfm_len);

/* tx */
int bflb_wifi_tx_eth(const uint8_t *frame, uint16_t len, uint8_t vif_idx, uint8_t sta_idx);
extern volatile uint32_t bflb_wifi_tx_status;

#endif /* BFLB_WIFI_IPC_H_ */
