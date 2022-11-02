/*
 * Copyright (c) 2022 Macronix International Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _BCH_H
#define _BCH_H

#include <stdint.h>

#define DIV_ROUND_UP(n, d)  (((n) + (d) - 1) / (d))

struct bch_code {
	int    m;
	int    n;
	int    t;
	int    ecc_bits;
	int    ecc_words;
	int    len;
	unsigned int   *a_pow;
	unsigned int   *a_log;
	unsigned int   *mod_tab;
	unsigned int   *ecc;
	unsigned int   *syn;
	unsigned int   *elp;
	unsigned int   *buf;
	unsigned int   *buf2;
	unsigned char   *input_data;
	int   endian;
};

struct bch_code *bch_init(int m, int t);
void bch_free(struct bch_code *bch);
void bch_encode(struct bch_code *bch, unsigned char *data, unsigned int *ecc);
int bch_decode(struct bch_code *bch, unsigned char *data, unsigned int *ecc);
int fls(int x);

#endif
