/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CRYPTO_CRYPTO_INTEL_SHA_PRIV_H_
#define ZEPHYR_DRIVERS_CRYPTO_CRYPTO_INTEL_SHA_PRIV_H_

#include <zephyr/kernel.h>
#include "crypto_intel_sha_registers.h"

#define SHA_HASH_DATA_BLOCK_LEN (64)
#define SHA_API_MAX_FRAG_LEN (64 * 1024 - 256)
#define SHA_REQUIRED_BLOCK_ALIGNMENT (512)

/* Possible SHA states */
#define SHA_FIRST (2)
#define SHA_MIDLE (3)
#define SHA_LAST (0)
/* SHA resume flag */
#define SHA_HRSM_ENABLE (1)
#define SHA_HRSM_DISABLE (0)

#define SHA1_ALGORITHM_HASH_SIZEOF (160 / 8)
#define SHA224_ALGORITHM_HASH_SIZEOF (224 / 8)
#define SHA256_ALGORITHM_HASH_SIZEOF (256 / 8)
#define SHA384_ALGORITHM_HASH_SIZEOF (384 / 8)
#define SHA512_ALGORITHM_HASH_SIZEOF (512 / 8)

#define SHA_MAX_SESSIONS 8

#define IS_ALIGNED(address, alignment) (((uintptr_t)(address)) % (alignment) == 0)
#define BYTE_SWAP32(x)                                                                             \
	(((x >> 24) & 0x000000FF) | ((x << 24) & 0xFF000000) | ((x >> 8) & 0x0000FF00) |           \
	 ((x << 8) & 0x00FF0000))

struct sha_hw_regs {
	union PIBCS pibcs;
	union PIBBA pibba;
	union PIBS pibs;
	union PIBFPI pibfpi;
	union PIBRP pibrp;
	union PIBWP pibwp;
	union PIBSP pibsp;
	uint32_t not_used1[5];
	union SHARLDW0 sharldw0;
	union SHARLDW1 sharldw1;
	union SHAALDW0 shaaldw0;
	union SHAALDW1 shaaldw1;
	union SHACTL shactl;
	union SHASTS shasts;
	uint32_t not_used12[2];
	uint8_t initial_vector[64];
	uint8_t sha_result[64];
};

union sha_state {
	uint32_t full;
	struct {
		/* Hash state: SHA_FIRST, SHA_MIDLE or SHA_LAST */
		uint32_t state : 3;
		/* Hash resume bit */
		uint32_t hrsm : 1;
		uint32_t rsvd : 28;
	} part;
};

struct sha_context {
	union SHAALDW0 shaaldw0;
	union SHAALDW1 shaaldw1;
	uint8_t initial_vector[SHA_HASH_DATA_BLOCK_LEN];
	uint8_t sha_result[SHA_HASH_DATA_BLOCK_LEN];
};

struct sha_session {
	struct sha_context sha_ctx;
	union sha_state state;
	uint32_t algo;
	bool in_use;
};

struct sha_container {
	/* pointer to DSP SHA Registers */
	volatile struct sha_hw_regs *dfsha;
};

#endif
