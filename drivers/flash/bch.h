/*
 * Generic binary BCH encoding/decoding library
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Copyright © 2011 Parrot S.A.
 *
 * Author: Ivan Djelic <ivan.djelic@parrot.com>
 *
 * Description:
 *
 * This library provides runtime configurable encoding/decoding of binary
 * Bose-Chaudhuri-Hocquenghem (BCH) codes.
*/
/* mbed Microcontroller Library
 * Copyright (c) 2022 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
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
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
//typedef unsigned char   uint8_t;
//typedef unsigned short  uint16_t;
//typedef unsigned int    uint32_t;

#define DIV_ROUND_UP(n, d)  (((n) + (d) - 1) / (d))
//#define ARRAY_SIZE(arr)     (sizeof(arr) / sizeof((arr)[0]))
/**
 * struct bch_control - BCH control structure
 * @m:          Galois field order
 * @n:          maximum codeword size in bits (= 2^m-1)
 * @t:          error correction capability in bits
 * @ecc_bits:   ecc exact size in bits, i.e. generator polynomial degree (<=m*t)
 * @ecc_bytes:  ecc max size (m*t bits) in bytes
 * @a_pow_tab:  Galois field GF(2^m) exponentiation lookup table
 * @a_log_tab:  Galois field GF(2^m) log lookup table
 * @mod8_tab:   remainder generator polynomial lookup tables
 * @ecc_buf:    ecc parity words buffer
 * @ecc_buf2:   ecc parity words buffer
 * @xi_tab:     GF(2^m) base for solving degree 2 polynomial roots
 * @syn:        syndrome buffer
 * @cache:      log-based polynomial representation buffer
 * @elp:        error locator polynomial
 * @poly_2t:    temporary polynomials of degree 2t
 */
struct bch_control {
    unsigned int    m;
    unsigned int    n;
    unsigned int    t;
    unsigned int    ecc_bits;
    unsigned int    ecc_bytes;
    /* private: */
    uint16_t       *a_pow_tab;
    uint16_t       *a_log_tab;
    uint32_t       *mod8_tab;
    uint32_t       *ecc_buf;
    uint32_t       *ecc_buf2;
    unsigned int   *xi_tab;
    unsigned int   *syn;
    int            *cache;
    struct gf_poly *elp;
    struct gf_poly *poly_2t[4];
};

struct bch_control *init_bch(int m, int t, unsigned int prim_poly);

void free_bch(struct bch_control *bch);

void encode_bch(struct bch_control *bch, const uint8_t *data,
                unsigned int len, uint8_t *ecc);

int decode_bch(struct bch_control *bch, const uint8_t *data, unsigned int len,
               const uint8_t *recv_ecc, const uint8_t *calc_ecc,
               const unsigned int *syn, unsigned int *errloc);
int fls(int x);
#ifdef _X86_
#define cpu_to_be32(x)  ((((x)&0xff)<<24) | (((x)&0xff00)<<8) | (((x)>>8)&0xff00) | (((x)>>24)&0xff))
#else
#define cpu_to_be32(x)  x
#endif
#ifdef __cplusplus
}
#endif
#endif /* _BCH_H */
