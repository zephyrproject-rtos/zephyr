/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INTEL_DAI_DRIVER_ALH_MAP_H__
#define __INTEL_DAI_DRIVER_ALH_MAP_H__

/**
 * \brief ALH Handshakes for audio I/O
 * Stream ID -> DMA Handshake map
 * -1 identifies invalid handshakes/streams
 */
static const uint8_t alh_handshake_map[64] = {
	-1,	/* 0  - INVALID */
	-1,	/* 1  - INVALID */
	-1,	/* 2  - INVALID */
	-1,	/* 3  - INVALID */
	-1,	/* 4  - INVALID */
	-1,	/* 5  - INVALID */
	-1,	/* 6  - INVALID */
	22,	/* 7  - BIDIRECTIONAL */
	23,	/* 8  - BIDIRECTIONAL */
	24,	/* 9  - BIDIRECTIONAL */
	25,	/* 10 - BIDIRECTIONAL */
	26,	/* 11 - BIDIRECTIONAL */
	27,	/* 12 - BIDIRECTIONAL */
	-1,	/* 13 - INVALID */
	-1,	/* 14 - INVALID */
	-1,	/* 15 - INVALID */
	-1,	/* 16 - INVALID */
	-1,	/* 17 - INVALID */
	-1,	/* 18 - INVALID */
	-1,	/* 19 - INVALID */
	-1,	/* 20 - INVALID */
	-1,	/* 21 - INVALID */
	-1,	/* 22 - INVALID */
	32,	/* 23 - BIDIRECTIONAL */
	33,	/* 24 - BIDIRECTIONAL */
	34,	/* 25 - BIDIRECTIONAL */
	35,	/* 26 - BIDIRECTIONAL */
	36,	/* 27 - BIDIRECTIONAL */
	37,	/* 28 - BIDIRECTIONAL */
	-1,	/* 29 - INVALID */
	-1,	/* 30 - INVALID */
	-1,	/* 31 - INVALID */
	-1,	/* 32 - INVALID */
	-1,	/* 33 - INVALID */
	-1,	/* 34 - INVALID */
	-1,	/* 35 - INVALID */
	-1,	/* 36 - INVALID */
	-1,	/* 37 - INVALID */
	-1,	/* 38 - INVALID */
	42,	/* 39 - BIDIRECTIONAL */
	43,	/* 40 - BIDIRECTIONAL */
	44,	/* 41 - BIDIRECTIONAL */
	45,	/* 42 - BIDIRECTIONAL */
	46,	/* 43 - BIDIRECTIONAL */
	47,	/* 44 - BIDIRECTIONAL */
	-1,	/* 45 - INVALID */
	-1,	/* 46 - INVALID */
	-1,	/* 47 - INVALID */
	-1,	/* 48 - INVALID */
	-1,	/* 49 - INVALID */
	-1,	/* 50 - INVALID */
	-1,	/* 51 - INVALID */
	-1,	/* 52 - INVALID */
	-1,	/* 53 - INVALID */
	-1,	/* 54 - INVALID */
	52,	/* 55 - BIDIRECTIONAL */
	53,	/* 56 - BIDIRECTIONAL */
	54,	/* 57 - BIDIRECTIONAL */
	55,	/* 58 - BIDIRECTIONAL */
	56,	/* 59 - BIDIRECTIONAL */
	57,	/* 60 - BIDIRECTIONAL */
	-1,	/* 61 - INVALID */
	-1,	/* 62 - INVALID */
	-1,	/* 63 - INVALID */
};

#endif
