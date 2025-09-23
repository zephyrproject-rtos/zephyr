/*
 * Copyright (c) 2022-2025 Macronix International Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _BCH_H
#define _BCH_H

#include <stdint.h>
#define ROUNDUP_DIV(_val, _base) (((_base) - 1 + (_val)) / (_base))

typedef struct _bch {
	bool le; /* little endian */
	uint32_t m;
	uint32_t n;
	uint32_t t;
	uint32_t ecc_bits;
	uint32_t ecc_words;
	uint32_t ecc_bytes;
	uint32_t len;
	uint32_t size_step;
	uint32_t *g; /* genpoly */
	uint16_t *a_pow;
	uint16_t *a_log;
	uint32_t *mod_tab;
	uint32_t *ecc;
	uint32_t *ecc2;
	uint32_t *syn;
	uint32_t *elp;
	uint32_t *buf;
	uint32_t *buf2;
	uint32_t *buf3;
} bch_t;

int bch_init(int m, int t, uint32_t size_step, bch_t **bch_ret);
void bch_free(bch_t *bch);
void bch_encode(bch_t *bch, uint8_t *data, uint8_t *ecc);
int bch_decode(bch_t *bch, uint8_t *data, uint8_t *ecc);
#endif
