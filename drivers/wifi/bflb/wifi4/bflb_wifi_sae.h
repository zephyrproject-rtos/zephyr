/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BFLB_WIFI_SAE_H_
#define BFLB_WIFI_SAE_H_

#include <stdint.h>
#include <stddef.h>

/* wpa_funcs wpa3 hooks: the firmware drives the SAE auth frames and asks
 * the host to build/parse the SAE payloads (type 1 = commit, 2 = confirm).
 */
uint8_t *bflb_wifi_sae_build(uint8_t *bssid, uint8_t *mac, uint8_t *passphrase, uint32_t type,
			     size_t *len);
int bflb_wifi_sae_parse(uint8_t *buf, size_t len, uint32_t type, uint16_t status);
void bflb_wifi_sae_clear(void);

#endif /* BFLB_WIFI_SAE_H_ */
