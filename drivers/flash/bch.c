/*
 * Copyright (c) 2022-2025 Macronix International Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "bch.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#ifndef CONFIG_SPI_NAND_BCH_HEAP_SIZE
#define CONFIG_SPI_NAND_BCH_HEAP_SIZE 51200
#endif

#define MAX_GEN_POLY_SIZE 169

K_HEAP_DEFINE(bch_heap, CONFIG_SPI_NAND_BCH_HEAP_SIZE);
LOG_MODULE_REGISTER(bch, CONFIG_FLASH_LOG_LEVEL);

static void *bch_alloc(size_t size, int *err)
{
	void *ptr = NULL;

	if (*err == 0) {
		ptr = k_heap_alloc(&bch_heap, size, K_NO_WAIT);
	}
	if (ptr == NULL) {
		*err = 1;
	}
	return ptr;
}

static inline uint32_t swap32_byte(uint32_t val)
{
	return (val & 0xff000000) >> 24 | (val & 0x00ff0000) >> 8 | (val & 0x0000ff00) << 8 |
	       (val & 0x000000ff) << 24;
}

static inline int mod(bch_t *bch, uint32_t v)
{
	while (v >= bch->n) {
		v -= bch->n;
		v = (v & bch->n) + (v >> bch->m);
	}
	return v;
}

static void build_syndrome(bch_t *bch)
{
	int i, j;
	int ecc_bits;
	uint32_t *ecc;

	memset(bch->syn, 0, 2 * bch->t * sizeof(*bch->syn));

	ecc_bits = bch->ecc_bits;
	ecc = bch->ecc;
	while (ecc_bits > 0) {
		i = ecc_bits - 32;
		ecc_bits = i;
		while (*ecc > 0) {
			if (*ecc & 1 != 0U) {
				for (j = 0; j < 2 * bch->t; j++) {
					bch->syn[j] ^= bch->a_pow[mod(bch, (j + 1) * i)];
				}
			}
			*ecc >>= 1;
			i++;
		}
		ecc++;
	}
}

static int build_error_location_poly(bch_t *bch)
{
	int i, j, k;
	int deg, buf_deg, tmp_deg;
	int pp = -1;
	uint32_t tmp, dp = 1, d = bch->syn[0];

	memset(bch->elp, 0, (bch->t + 1) * sizeof(*bch->elp));

	buf_deg = 0;
	bch->buf[0] = 1;
	deg = 0;
	bch->elp[0] = 1;

	for (i = 0; (i < bch->t) && (deg <= bch->t); i++) {
		if (d != 0) {
			k = 2 * i - pp;
			if (buf_deg + k > deg) {
				tmp_deg = deg;
				for (j = 0; j <= deg; j++) {
					bch->buf2[j] = bch->elp[j];
				}
			}
			tmp = bch->n + bch->a_log[d] - bch->a_log[dp];

			for (j = 0; j <= buf_deg; j++) {
				if (bch->buf[j] != 0) {
					bch->elp[j + k] ^=
						bch->a_pow[mod(bch, tmp + bch->a_log[bch->buf[j]])];
				}
			}
			if (buf_deg + k > deg) {
				deg = buf_deg + k;
				buf_deg = tmp_deg;
				for (j = 0; j <= tmp_deg; j++) {
					bch->buf[j] = bch->buf2[j];
				}
				dp = d;
				pp = 2 * i;
			}
		}
		if (i < bch->t - 1) {
			k = 2 * i + 1;
			d = bch->syn[k + 1];
			for (j = 1; j <= deg; j++) {
				if (bch->elp[j] && bch->syn[k]) {
					d ^= bch->a_pow[mod(bch, bch->a_log[bch->elp[j]] +
									 bch->a_log[bch->syn[k]])];
				}
				k--;
			}
		}
	}
	return (deg > bch->t) ? -1 : deg;
}

static int chien_search(bch_t *bch, int deg)
{
	int i, j, k, nroot = 0;
	int *rep = (int *)bch->buf;
	int *root = (int *)bch->buf2;
	uint32_t syn, syn0;

	k = bch->n - bch->a_log[bch->elp[deg]];
	for (i = 0; i < deg; i++) {
		rep[i] = bch->elp[i] ? mod(bch, bch->a_log[bch->elp[i]] + k) : -1;
	}
	rep[i] = 0;

	syn0 = bch->elp[0] ? bch->a_pow[rep[0]] : 0;
	for (i = 0; i <= bch->n; i++) {
		for (j = 1, syn = syn0; j <= deg; j++) {
			if (rep[j] >= 0) {
				syn ^= bch->a_pow[mod(bch, rep[j] + j * i)];
			}
		}
		if (syn == 0) {
			root[nroot++] = bch->n - i;
			if (nroot == deg) {
				return nroot;
			}
		}
	}
	return 0;
}

static void build_gf_table(bch_t *bch)
{
	uint32_t x;
	uint32_t msb, poly;
	uint32_t prim_poly[] = {0x11d, 0x211, 0x409, 0x805, 0x1053, 0x201b};

	poly = prim_poly[bch->m - 8];
	msb = 1 << bch->m;
	bch->a_pow[0] = 1;
	bch->a_log[1] = 0;
	x = 2;
	for (int i = 1; i < bch->n; i++) {
		bch->a_pow[i] = x;
		bch->a_log[x] = i;
		x <<= 1;
		if (x & msb != 0U) {
			x ^= poly;
		}
	}
	bch->a_pow[bch->n] = 1;
	bch->a_log[0] = 0;
}

static void build_mod_tables(bch_t *bch, const uint32_t *g)
{
	int i, j, b, d;
	int plen = (bch->ecc_bits + 32) / 32;
	int ecclen = (bch->ecc_bits + 31) / 32;
	uint32_t data, hi, lo, *tab, poly;

	memset(bch->mod_tab, 0, 16 * 4 * bch->ecc_words * sizeof(*bch->mod_tab));

	for (i = 0; i < 4; i++) {
		for (b = 0; b < 16; b++) {
			tab = bch->mod_tab + (b * 4 + i) * bch->ecc_words;
			data = i << (2 * b);
			while (data) {
				d = 0;
				poly = (data >> 1);
				while (poly) {
					poly >>= 1;
					d++;
				}
				data ^= g[0] >> (31 - d);
				for (j = 0; j < ecclen; j++) {
					hi = (d < 31) ? g[j] << (d + 1) : 0;
					lo = (j + 1 < plen) ? g[j + 1] >> (31 - d) : 0;
					tab[j] ^= hi | lo;
				}
			}
		}
	}
}

static int build_generator_poly(bch_t *bch)
{
	int i, j, k, m, t;
	uint32_t n;
	uint32_t x[MAX_GEN_POLY_SIZE] = {0};

	for (t = 0, x[0] = 1, bch->ecc_bits = 0; t < bch->t; t++) {
		for (m = 0, i = 2 * t + 1; m < bch->m; m++) {
			x[bch->ecc_bits + 1] = 1;
			for (j = bch->ecc_bits; j > 0; j--) {
				if (x[j] != 0) {
					x[j] = bch->a_pow[mod(bch, bch->a_log[x[j]] + i)] ^
					       x[j - 1];
				} else {
					x[j] = x[j - 1];
				}
			}
			if (x[j] != 0) {
				x[j] = bch->a_pow[mod(bch, bch->a_log[x[j]] + i)];
			}
			bch->ecc_bits++;
			i = mod(bch, 2 * i);
		}
	}

	for (k = bch->ecc_bits + 1, i = 0; k > 0; k = k - n) {
		n = (k > 32) ? 32 : k;
		for (j = 0; j < n; j++) {
			if (x[k - 1 - j] != 0) {
				bch->g[i] |= 1U << (31 - j);
			}
		}
		i++;
	}

	return 0;
}

void bch_encode(bch_t *bch, uint8_t *data, uint8_t *ecc)
{
	int i, j, k, mlen;
	uint32_t w;
	uint32_t *p;
	uint32_t *c[16];
	uint32_t *t[16];

	memset(bch->ecc, 0, bch->ecc_words * sizeof(*bch->ecc));
	memset(bch->buf3, 0, bch->len);
	memcpy(bch->buf3 + bch->ecc_words, data, bch->size_step);

	t[0] = bch->mod_tab;
	for (i = 1; i < 16; i++) {
		t[i] = t[i - 1] + 4 * (bch->ecc_words);
	}
	p = bch->buf3;
	mlen = bch->len / 4;

	while (mlen-- > 0) {
		w = bch->le ? swap32_byte(*p) : *p;
		p++;

		w ^= bch->ecc[0];

		k = 0;
		for (i = 0; i < 16; i++) {
			c[i] = t[i] + (bch->ecc_words) * ((w >> k) & 0x03);
			k = k + 2;
		}

		for (i = 0; i < bch->ecc_words - 1; i++) {
			bch->ecc[i] = bch->ecc[i + 1];
			for (j = 0; j < 16; j++) {
				bch->ecc[i] ^= c[j][i];
			}
		}
		bch->ecc[i] = c[0][i];
		for (j = 1; j < 16; j++) {
			bch->ecc[i] ^= c[j][i];
		}
	}

	if (ecc != NULL) {
		for (i = 0; i < bch->ecc_words; i++) {
			bch->ecc2[i] = swap32_byte(bch->ecc[i]);
		}

		memcpy(ecc, bch->ecc2, bch->ecc_bytes);
	}
}

int bch_decode(bch_t *bch, uint8_t *data, uint8_t *ecc)
{
	int i, err = 0, nroot;
	int *root = (int *)bch->buf2;
	uint32_t nbits;

	bch_encode(bch, data, NULL);
	memcpy(bch->ecc2, ecc, bch->ecc_bytes);
	for (i = 0; i < bch->ecc_words; i++) {
		uint32_t ecc_word_le = swap32_byte(bch->ecc2[i]);

		LOG_ERR("<word %d> %08X : %08X %s\r\n", i, bch->ecc[i], ecc_word_le,
			bch->ecc[i] != ecc_word_le ? "**" : "");
		bch->ecc[i] ^= ecc_word_le;
		err |= bch->ecc[i];
	}
	if (err == 0) {
		return 0;
	}
	build_syndrome(bch);
	err = build_error_location_poly(bch);
	if (err <= 0) {
		return -1;
	}
	nroot = chien_search(bch, err);
	if (err != nroot) {
		return -1;
	}
	nbits = (bch->len * 8) + bch->ecc_bits;
	for (i = 0; i < err; i++) {
		root[i] = nbits - 1 - root[i];
		root[i] = (root[i] & ~7) | (7 - (root[i] & 7));

		if ((root[i] / 8) < (bch->ecc_words * 4)) {
			LOG_WRN("Error bit is in ecc range form byte 0 to %d, SKIP!!\r\n",
				root[i] / 8);
			continue;
		}
		LOG_ERR("Before correct: <%d> %02X\r\n", (root[i] / 8) - (bch->ecc_words * 4),
			data[(root[i] / 8) - (bch->ecc_words * 4)]);

		data[(root[i] / 8) - (bch->ecc_words * 4)] ^= 1 << root[i] % 8;

		LOG_ERR("After  correct: <%d> %02X\r\n", (root[i] / 8) - (bch->ecc_words * 4),
			data[(root[i] / 8) - (bch->ecc_words * 4)]);
	}

	return err;
}

int bch_init(int m, int t, uint32_t size_step, bch_t **bch_ret)
{
	bch_t *bch;
	int err = 0;
	uint32_t a = 1;
	uint8_t *p = (uint8_t *)&a;
	*bch_ret = 0;

	if ((m < 8) || (m > 13) || (t < 1) || (t > 12)) {
		LOG_DBG("bch init failed, params should be m: 8 ~ 13, t: 1 ~ 12\r\n");
		return -EINVAL;
	}
	bch = bch_alloc(sizeof(bch_t), &err);
	if (bch == NULL) {
		return -ENOMEM;
	}
	bch->m = m;
	bch->t = t;
	bch->n = (1 << m) - 1;
	bch->ecc_words = (m * t + 31) / 32;
	bch->len = (bch->n + 1) / 8;
	bch->a_pow = bch_alloc((bch->n + 1) * sizeof(*bch->a_pow), &err);
	bch->a_log = bch_alloc((bch->n + 1) * sizeof(*bch->a_log), &err);
	bch->mod_tab = bch_alloc(bch->ecc_words * 16 * 4 * sizeof(*bch->mod_tab), &err);
	bch->ecc = bch_alloc(bch->ecc_words * sizeof(*bch->ecc), &err);
	bch->ecc2 = bch_alloc(bch->ecc_words * sizeof(*bch->ecc2), &err);
	bch->buf = bch_alloc((t + 1) * sizeof(*bch->buf), &err);
	bch->buf2 = bch_alloc((t + 1) * sizeof(*bch->buf2), &err);
	bch->buf3 = bch_alloc(bch->len, &err);
	bch->syn = bch_alloc(t * 2 * sizeof(*bch->syn), &err);
	bch->elp = bch_alloc((t + 1) * sizeof(*bch->elp), &err);
	bch->g = bch_alloc((bch->ecc_words + 1) * sizeof(*bch->g), &err);

	if (err != 0) {
		bch_free(bch);
		return -ENOMEM;
	}

	bch->le = (*p == 1);
	LOG_DBG("This system is %s endian\r\n", bch->le ? "Little" : "Big");

	bch->size_step = size_step;

	build_gf_table(bch);
	if (0 != build_generator_poly(bch)) {
		bch_free(bch);
		return -EINVAL;
	}
	bch->ecc_bytes = (bch->ecc_bits + 7) / 8;
	build_mod_tables(bch, bch->g);

	*bch_ret = bch;

	return 0;
}

void bch_free(bch_t *bch)
{
	if (bch != NULL) {
		k_heap_free(&bch_heap, bch->a_pow);
		k_heap_free(&bch_heap, bch->a_log);
		k_heap_free(&bch_heap, bch->mod_tab);
		k_heap_free(&bch_heap, bch->ecc);
		k_heap_free(&bch_heap, bch->ecc2);
		k_heap_free(&bch_heap, bch->syn);
		k_heap_free(&bch_heap, bch->elp);
		k_heap_free(&bch_heap, bch->buf);
		k_heap_free(&bch_heap, bch->buf2);
		k_heap_free(&bch_heap, bch->buf3);
		k_heap_free(&bch_heap, bch->g);
		k_heap_free(&bch_heap, bch);
	}
}
