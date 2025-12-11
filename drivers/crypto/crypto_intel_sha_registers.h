/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CRYPTO_CRYPTO_INTEL_SHA_REGISTERS_PRIV_H_
#define ZEPHYR_DRIVERS_CRYPTO_CRYPTO_INTEL_SHA_REGISTERS_PRIV_H_

#include <stdio.h>

union PIBCS {
	uint32_t full;
	struct {
		uint32_t rsvd2 : 3;
		uint32_t bscie : 1;
		uint32_t rsvd4 : 1;
		uint32_t rsvd5 : 1;
		uint32_t teie : 1;
		uint32_t rsvd7 : 1;
		uint32_t bne : 1;
		uint32_t bf : 1;
		uint32_t rsvd10 : 1;
		uint32_t bsc : 1;
		uint32_t rsvd13 : 2;
		uint32_t te : 1;
		uint32_t rsvd15 : 1;
		uint32_t cs : 7;
		uint32_t fwcb : 1;
		uint32_t rsvd25 : 2;
		uint32_t peen : 1;
		uint32_t rsvd31 : 5;
	} part;
};

union PIBBA {
	uint32_t full;
	struct {
		uint32_t rsvd6 : 7;
		uint32_t ba : 17;
		uint32_t rsvd31 : 8;
	} part;
};

union PIBS {
	uint32_t full;
	struct {
		uint32_t rsvd3 : 4;
		uint32_t bs : 20;
		uint32_t rsvd31 : 8;
	} part;
};

union PIBFPI {
	uint32_t full;
	struct {
		uint32_t wp : 24;
		uint32_t rsvd31 : 8;
	} part;
};

union PIBRP {
	uint32_t full;
	struct {
		uint32_t rp : 24;
		uint32_t rsvd31 : 8;
	} part;
};

union PIBWP {
	uint32_t full;
	struct {
		uint32_t wp : 24;
		uint32_t rsvd31 : 8;
	} part;
};

union PIBSP {
	uint32_t full;
	struct {
		uint32_t rp : 24;
		uint32_t rsvd31 : 8;
	} part;
};

union SHARLDW0 {
	uint32_t full;
	struct {
		uint32_t rsvd8 : 9;
		uint32_t lower_length : 23;
	} part;
};

union SHARLDW1 {
	uint32_t full;
	struct {
		uint32_t upper_length : 32;
	} part;
};

union SHAALDW0 {
	uint32_t full;
	struct {
		uint32_t rsvd8 : 9;
		uint32_t lower_length : 23;
	} part;
};

union SHAALDW1 {
	uint32_t full;
	struct {
		uint32_t upper_length : 32;
	} part;
};

union SHACTL {
	uint32_t full;
	struct {
		uint32_t en : 1;
		uint32_t rsvd1 : 1;
		uint32_t hrsm : 1;
		uint32_t hfm : 2;
		uint32_t rsvd15 : 11;
		uint32_t algo : 3;
		uint32_t rsvd31 : 13;
	} part;
};

union SHASTS {
	uint32_t full;
	struct {
		uint32_t busy : 1;
		uint32_t rsvd31 : 31;
	} part;
};

union SHAIVDWx {
	uint32_t full;
	struct {
		uint32_t dwx : 32;
	} part;
};

union SHARDWx {
	uint32_t full;
	struct {
		uint32_t dwx : 32;
	} part;
};

#endif
