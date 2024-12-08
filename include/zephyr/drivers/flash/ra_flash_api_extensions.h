/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZEPHYR_INCLUDE_DRIVERS_FLASH_RA_EXTENSIONS_H__
#define __ZEPHYR_INCLUDE_DRIVERS_FLASH_RA_EXTENSIONS_H__

#include <zephyr/drivers/flash.h>

enum ra_ex_ops {
	FLASH_RA_EX_OP_WRITE_PROTECT = FLASH_EX_OP_VENDOR_BASE,
};

typedef struct {
	union {
		uint32_t BPS[4];

		struct {
			uint32_t b000: 1;
			uint32_t b001: 1;
			uint32_t b002: 1;
			uint32_t b003: 1;
			uint32_t b004: 1;
			uint32_t b005: 1;
			uint32_t b006: 1;
			uint32_t b007: 1;
			uint32_t b008: 1;
			uint32_t b009: 1;
			uint32_t b010: 1;
			uint32_t b011: 1;
			uint32_t b012: 1;
			uint32_t b013: 1;
			uint32_t b014: 1;
			uint32_t b015: 1;
			uint32_t b016: 1;
			uint32_t b017: 1;
			uint32_t b018: 1;
			uint32_t b019: 1;
			uint32_t b020: 1;
			uint32_t b021: 1;
			uint32_t b022: 1;
			uint32_t b023: 1;
			uint32_t b024: 1;
			uint32_t b025: 1;
			uint32_t b026: 1;
			uint32_t b027: 1;
			uint32_t b028: 1;
			uint32_t b029: 1;
			uint32_t b030: 1;
			uint32_t b031: 1;
			uint32_t b032: 1;
			uint32_t b033: 1;
			uint32_t b034: 1;
			uint32_t b035: 1;
			uint32_t b036: 1;
			uint32_t b037: 1;
			uint32_t b038: 1;
			uint32_t b039: 1;
			uint32_t b040: 1;
			uint32_t b041: 1;
			uint32_t b042: 1;
			uint32_t b043: 1;
			uint32_t b044: 1;
			uint32_t b045: 1;
			uint32_t b046: 1;
			uint32_t b047: 1;
			uint32_t b048: 1;
			uint32_t b049: 1;
			uint32_t b050: 1;
			uint32_t b051: 1;
			uint32_t b052: 1;
			uint32_t b053: 1;
			uint32_t b054: 1;
			uint32_t b055: 1;
			uint32_t b056: 1;
			uint32_t b057: 1;
			uint32_t b058: 1;
			uint32_t b059: 1;
			uint32_t b060: 1;
			uint32_t b061: 1;
			uint32_t b062: 1;
			uint32_t b063: 1;
			uint32_t b064: 1;
			uint32_t b065: 1;
			uint32_t b066: 1;
			uint32_t b067: 1;
			uint32_t b068: 1;
			uint32_t b069: 1;
			uint32_t b070: 1;
			uint32_t b071: 1;
			uint32_t b072: 1;
			uint32_t b073: 1;
			uint32_t b074: 1;
			uint32_t b075: 1;
			uint32_t b076: 1;
			uint32_t b077: 1;
			uint32_t b078: 1;
			uint32_t b079: 1;
			uint32_t b080: 1;
			uint32_t b081: 1;
			uint32_t b082: 1;
			uint32_t b083: 1;
			uint32_t b084: 1;
			uint32_t b085: 1;
			uint32_t b086: 1;
			uint32_t b087: 1;
			uint32_t b088: 1;
			uint32_t b089: 1;
			uint32_t b090: 1;
			uint32_t b091: 1;
			uint32_t b092: 1;
			uint32_t b093: 1;
			uint32_t b094: 1;
			uint32_t b095: 1;
			uint32_t b096: 1;
			uint32_t b097: 1;
			uint32_t b098: 1;
			uint32_t b099: 1;
			uint32_t b100: 1;
			uint32_t b101: 1;
			uint32_t b102: 1;
			uint32_t b103: 1;
			uint32_t b104: 1;
			uint32_t b105: 1;
			uint32_t b106: 1;
			uint32_t: 21;
		} BPS_b;
	};
} flash_ra_cf_block_map;

#if defined(CONFIG_FLASH_RA_WRITE_PROTECT)
typedef struct flash_ra_ex_write_protect_in {
	flash_ra_cf_block_map protect_enable;
	flash_ra_cf_block_map protect_disable;
	flash_ra_cf_block_map protect_permanent;
} flash_ra_ex_write_protect_in_t;

typedef struct flash_ra_ex_write_protect_out {
	flash_ra_cf_block_map protected_enabled;
	flash_ra_cf_block_map protected_premanent;
} flash_ra_ex_write_protect_out_t;
#endif /* CONFIG_FLASH_RA_WRITE_PROTECT */

#endif
