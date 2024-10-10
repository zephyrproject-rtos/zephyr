/*
 * Copyright (c) 2022 Macronix International Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "bch.h"
#include <zephyr/drivers/spi.h>

int find_last_set(int x)
{
    int r = 32;

    if (!x) {
        return 0;
    }
    if (!(x & 0xffff0000u)) {
        x <<= 16;
        r -= 16;
    }
    if (!(x & 0xff000000u)) {
        x <<= 8;
        r -= 8;
    }
    if (!(x & 0xf0000000u)) {
        x <<= 4;
        r -= 4;
    }
    if (!(x & 0xc0000000u)) {
        x <<= 2;
        r -= 2;
    }
    if (!(x & 0x80000000u)) {
        x <<= 1;
        r -= 1;
    }
    return r;
}

void bch_encode(struct bch_code *bch, unsigned char *data, unsigned int *ecc)
{
	int i, j, k, mlen;
	unsigned int w;
	unsigned int *p;
	unsigned int *c[16];
	unsigned int *t[16];

	t[0] = bch->mod_tab;
	for (i = 1; i < 16; i++)
		t[i] = t[i-1] + 4 * (bch->ecc_words);

	memset(bch->ecc, 0, bch->ecc_words * sizeof(*bch->ecc));

	p = (unsigned int *)data;
	mlen  = bch->len / 4;

	while (mlen--) {
		if (!bch->endian) {
			w = ((unsigned int)(*p) & 0xff000000) >> 24 |
			    ((unsigned int)(*p) & 0x00ff0000) >> 8  |
			    ((unsigned int)(*p) & 0x0000ff00) << 8  |
			    ((unsigned int)(*p) & 0x000000ff) << 24;
		} else {
			w = *p;
		}
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
			ecc[i] = bch->ecc[i];
		}
	}
}

static inline int mod(struct bch_code *bch, unsigned int v)
{
	while (v >= bch->n) {
		v -= bch->n;
		v = (v & bch->n) + (v >> bch->m);
	}
	return v;
}

static void build_syndrome(struct bch_code *bch)
{
	int i, j;
	int ecc_bits;
	unsigned int *ecc;

	memset(bch->syn, 0, 2 * bch->t * sizeof(*bch->syn));

	ecc_bits = bch->ecc_bits;
	ecc = bch->ecc;
	while (ecc_bits > 0) {
		i = ecc_bits - 32;
		ecc_bits = i;
		while (*ecc) {
			if (*ecc & (unsigned int)1) {
			for (j = 0; j < 2*bch->t; j++)
				bch->syn[j] ^= bch->a_pow[mod(bch, (j+1)*i)];
			}
			*ecc >>= 1;
			i++;
		}
		ecc++;
	}
}

static int build_error_location_poly(struct bch_code *bch)
{
	int i, j, k;
	unsigned int tmp, dp = 1, d = bch->syn[0];
	int deg, buf_deg, tmp_deg;
	int pp = -1;

	memset(bch->elp, 0, (bch->t + 1) * sizeof(*bch->elp));

	buf_deg = 0;
	bch->buf[0] = 1;
	deg = 0;
	bch->elp[0] = 1;

	for (i = 0; (i < bch->t) && (deg <= bch->t); i++) {
		if (d) {
			k = 2 * i-pp;
			if (buf_deg + k > deg) {
				tmp_deg = deg;
				for (j = 0; j <= deg; j++) {
					bch->buf2[j] = bch->elp[j];
				}
			}
			tmp = bch->n + bch->a_log[d] - bch->a_log[dp];

			for (j = 0; j <= buf_deg; j++) {
				if (bch->buf[j]) {
					bch->elp[j+k] ^= bch->a_pow[mod(bch, tmp + bch->a_log[bch->buf[j]])];
				}
			}
			if (buf_deg+k > deg) {
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
					d ^= bch->a_pow[mod(bch, bch->a_log[bch->elp[j]] + bch->a_log[bch->syn[k]])];
				}
				k--;
			}
		}
	}
	return (deg > bch->t) ? -1 : deg;
}

static int chien_search(struct bch_code *bch, int deg)
{
	int i, j, k, nroot = 0;
	unsigned int syn, syn0;
	int *rep  = (int*) bch->buf;
	int *root = (int*) bch->buf2;

	k = bch->n - bch->a_log[bch->elp[deg]];
	for (i = 0; i < deg; i++) {
		rep[i] = bch->elp[i] ? mod(bch, bch->a_log[ bch->elp[i] ] + k) : -1;
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

int bch_decode(struct bch_code *bch, unsigned char *data, unsigned int *ecc)
{
	unsigned int nbits;
	int i, err, nroot;
	int *root = (int*) bch->buf2;

	bch_encode(bch, data, NULL);

	for (i = 0, err = 0; i < bch->ecc_words; i++) {
		bch->ecc[i] ^= ecc[i];
		err |= bch->ecc[i];
	}
	if (!err)
		return 0;

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
		data[root[i]/8] ^= 1 << root[i]%8;
	}

	return err;
}

static void build_gf_table(struct bch_code *bch)
{
	unsigned int i, x;
	unsigned int msb, poly;
	unsigned int prim_poly[5] = {0x11d, 0x211, 0x409, 0x805, 0x1053};

	poly = prim_poly[bch->m - 8];
	msb = 1 << bch->m;
	bch->a_pow[0] = 1;
	bch->a_log[1] = 0;
	x = 2;
	for (i = 1; i < bch->n; i++) {
		bch->a_pow[i] = x;
		bch->a_log[x] = i;
		x <<= 1;
		if (x & msb) {
			x ^= poly;
		}
	}
	bch->a_pow[bch->n] = 1;
	bch->a_log[0] = 0;
}

static void build_mod_tables(struct bch_code *bch, const unsigned int *g)
{
	int i, j, b, d;
	unsigned int data, hi, lo, *tab, poly;
	int plen = (bch->ecc_bits + 32) / 32;
	int ecclen = (bch->ecc_bits + 31) / 32;

	memset(bch->mod_tab, 0, 16 * 4 * bch->ecc_words * sizeof(*bch->mod_tab));

	for (i = 0; i < 4; i++) {
		for (b = 0; b < 16; b++) {
			tab = bch->mod_tab + (b*4+i)*bch->ecc_words;
			data = i << (2*b);
			while (data) {
				d = 0;
				poly = (data >> 1);
				while(poly) {
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

static void *bch_alloc(size_t size, int *err)
{
	void *ptr = NULL;
	if (*err == 0)
		ptr = k_malloc(size);
	if (ptr == NULL)
		*err = 1;
	return ptr;
}

static unsigned int *build_generator_poly(struct bch_code *bch)
{
	int i, j, k;
	int m, t;
	int err = 0;
	unsigned int n;
	unsigned int *x;
	unsigned int *g;

	x = bch_alloc((bch->m * bch->t + 1) * sizeof(*x), &err);
	g = bch_alloc((bch->ecc_words + 1)  * sizeof(*g), &err);

	if (err) {
		k_free(g);
		k_free(x);
		bch_free(bch);
		return NULL;
	}

	bch->ecc_bits = 0;
	x[0] = 1;
	for (t = 0; t < bch->t; t++) {
		for (m = 0, i = 2 * t + 1; m < bch->m; m++) {
			x[bch->ecc_bits + 1] = 1;
			for (j = bch->ecc_bits; j > 0; j--) {
				if (x[j]) {
					x[j] = bch->a_pow[mod(bch, bch->a_log[x[j]] + i)] ^ x[j - 1];
				} else {
					x[j] = x[j-1];
				}
			}
			if (x[j])
				x[j] = bch->a_pow[mod(bch, bch->a_log[x[j]] + i)];
			bch->ecc_bits++;
			i = mod(bch, 2 * i);
		}
	}

	i = 0;
	memset(g, 0, (bch->ecc_words + 1) * sizeof(*g));

	for (k = bch->ecc_bits + 1; k > 0; k = k - n) {
		n = (k > 32) ? 32 : k;
		for (j = 0; j < n; j++) {
			if (x[k-1-j])
				g[i] |= (unsigned int)1 << (31 - j);
		}
		i++;
	}

	k_free(x);
	return g;
}

struct bch_code *bch_init(int m, int t)
{
	int err = 0;
	unsigned int *genpoly;
	struct bch_code *bch = NULL;

	short int a = 0x1234;
	char *p = (char *)&a;

	if ((m < 8) || (m > 12)) {
		return NULL;
	}
	if ((t < 1) || (t > 12)) {
		return NULL;
	}

	bch = (struct bch_code *)k_malloc(sizeof(struct bch_code));

	if (bch == NULL) {
		return NULL;
	}

	bch->m = m;
	bch->t = t;
	bch->n = (1 << m)-1;
	bch->ecc_words  = (m * t + 31) / 32;
	bch->len        = (bch->n + 1) / 8;
	bch->a_pow      = bch_alloc((1 + bch->n)*sizeof(*bch->a_pow), &err);
	bch->a_log      = bch_alloc((1 + bch->n)*sizeof(*bch->a_log), &err);
	bch->mod_tab    = bch_alloc(bch->ecc_words * 16 * 4 * sizeof(*bch->mod_tab), &err);
	bch->ecc        = bch_alloc(bch->ecc_words * sizeof(*bch->ecc), &err);
	bch->syn        = bch_alloc(2 * t * sizeof(*bch->syn), &err);
	bch->elp        = bch_alloc((t + 1) * sizeof(*bch->elp), &err);
	bch->buf        = bch_alloc((t + 1) * sizeof(*bch->buf), &err);
	bch->buf2       = bch_alloc((t + 1) * sizeof(*bch->buf2), &err);
	bch->input_data = bch_alloc((1 << m) / 8, &err);

	if (*p == 0x34) {
		bch->endian = 0;
	} else if (*p == 0x12) {
		bch->endian = 1;
	} else {
		err = 1;
	}

	if (err) {
		bch_free(bch);
		return NULL;
	}

	build_gf_table(bch);
	genpoly = build_generator_poly(bch);
	if (genpoly == NULL) {
		return NULL;
	}

	build_mod_tables(bch, genpoly);
	k_free(genpoly);

	if (err) {
		bch_free(bch);
		return NULL;
	}

	return bch;
}

void bch_free(struct bch_code *bch)
{
	if (bch) {
		k_free(bch->a_pow);
		k_free(bch->a_log);
		k_free(bch->mod_tab);
		k_free(bch->ecc);
		k_free(bch->syn);
		k_free(bch->elp);
		k_free(bch->buf);
		k_free(bch->buf2);
		k_free(bch);
	}
}