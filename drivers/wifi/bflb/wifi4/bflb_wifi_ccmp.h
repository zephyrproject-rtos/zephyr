/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BFLB_WIFI_CCMP_H_
#define BFLB_WIFI_CCMP_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

int bflb_wifi_ccmp_set_key(const uint8_t *tk);
void bflb_wifi_ccmp_clear(void);
int bflb_wifi_ccmp_encrypt(uint8_t *out, size_t out_size, const uint8_t *mac_hdr,
			   size_t mac_hdr_len, const uint8_t *plain, size_t plain_len, int qos_tid);

#endif /* BFLB_WIFI_CCMP_H_ */
