/*
 * Copyright (c) 2016-2020 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DOUBLE_BUFFER_SIZE
#define DOUBLE_BUFFER_SIZE 2
#endif

#ifndef TRIPLE_BUFFER_SIZE
#define TRIPLE_BUFFER_SIZE 3
#endif

uint8_t util_ones_count_get(const uint8_t *octets, uint8_t octets_len);
int util_aa_le32(uint8_t *dst);
int util_saa_le32(uint8_t *dst, uint8_t handle);
void util_bis_aa_le32(uint8_t bis, uint8_t *saa, uint8_t *dst);
uint32_t util_get_bits(uint8_t *data, uint8_t bit_offs, uint8_t num_bits);
void util_set_bits(uint8_t *data, uint8_t bit_offs, uint8_t num_bits,
		   uint32_t value);
