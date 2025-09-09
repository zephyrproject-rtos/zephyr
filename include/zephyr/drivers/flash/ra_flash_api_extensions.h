/*
 * Copyright (c) 2024-2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for Renesas RA flash extended operations.
 * @ingroup ra_flash_ex_op
 */

#ifndef __ZEPHYR_INCLUDE_DRIVERS_FLASH_RA_FLASH_API_EXTENSIONS_H__
#define __ZEPHYR_INCLUDE_DRIVERS_FLASH_RA_FLASH_API_EXTENSIONS_H__

/**
 * @brief Extended operations for Renesas RA flash controllers.
 * @defgroup ra_flash_ex_op Renesas RA
 * @ingroup flash_ex_op
 * @{
 */

#include <zephyr/drivers/flash.h>

/**
 * @brief Enumeration for Renesas RA flash extended operations.
 */
enum ra_ex_ops {
	/**
	 * Renesas RA flash write protection control.
	 *
	 * @kconfig_dep{CONFIG_FLASH_RENESAS_RA_HP_WRITE_PROTECT}
	 *
	 * @param in Pointer to a @ref flash_ra_ex_write_protect_in_t structure specifying the
	 *           desired changes to the write protection settings. Can be @a NULL if only
	 *           retrieving the current write protection settings is desired.
	 * @param out Pointer to a @ref flash_ra_ex_write_protect_out_t structure to store the
	 *            current write protection settings. Can be @a NULL if the statis is not needed.
	 */
	FLASH_RA_EX_OP_WRITE_PROTECT = FLASH_EX_OP_VENDOR_BASE,
	/**
	 * Reset Flash device (at QPI(4-4-4) mode).
	 */
	QSPI_FLASH_EX_OP_EXIT_QPI,
};

/**
 * @brief A bitmask structure for mapping code flash blocks.
 *
 * This structure provides a bitfield representation for the code flash blocks,
 * allowing individual blocks to be selected for operations like write protection.
 * Each bit from b000 to b106 corresponds to a specific code flash block.

 * Setting a bit to '1' in a mask selects that block for an operation.
 */
typedef struct {
	/** Union allowing access to the block map as an array of 32-bit words or as bitfields. */
	union {
		/** Access the block map as an array of 32-bit words. */
		uint32_t BPS[4];

		/** Access individual blocks as bitfields. */
		struct {
			uint32_t b000: 1; /**< Block 0 */
			uint32_t b001: 1; /**< Block 1 */
			uint32_t b002: 1; /**< Block 2 */
			uint32_t b003: 1; /**< Block 3 */
			uint32_t b004: 1; /**< Block 4 */
			uint32_t b005: 1; /**< Block 5 */
			uint32_t b006: 1; /**< Block 6 */
			uint32_t b007: 1; /**< Block 7 */
			uint32_t b008: 1; /**< Block 8 */
			uint32_t b009: 1; /**< Block 9 */
			uint32_t b010: 1; /**< Block 10 */
			uint32_t b011: 1; /**< Block 11 */
			uint32_t b012: 1; /**< Block 12 */
			uint32_t b013: 1; /**< Block 13 */
			uint32_t b014: 1; /**< Block 14 */
			uint32_t b015: 1; /**< Block 15 */
			uint32_t b016: 1; /**< Block 16 */
			uint32_t b017: 1; /**< Block 17 */
			uint32_t b018: 1; /**< Block 18 */
			uint32_t b019: 1; /**< Block 19 */
			uint32_t b020: 1; /**< Block 20 */
			uint32_t b021: 1; /**< Block 21 */
			uint32_t b022: 1; /**< Block 22 */
			uint32_t b023: 1; /**< Block 23 */
			uint32_t b024: 1; /**< Block 24 */
			uint32_t b025: 1; /**< Block 25 */
			uint32_t b026: 1; /**< Block 26 */
			uint32_t b027: 1; /**< Block 27 */
			uint32_t b028: 1; /**< Block 28 */
			uint32_t b029: 1; /**< Block 29 */
			uint32_t b030: 1; /**< Block 30 */
			uint32_t b031: 1; /**< Block 31 */
			uint32_t b032: 1; /**< Block 32 */
			uint32_t b033: 1; /**< Block 33 */
			uint32_t b034: 1; /**< Block 34 */
			uint32_t b035: 1; /**< Block 35 */
			uint32_t b036: 1; /**< Block 36 */
			uint32_t b037: 1; /**< Block 37 */
			uint32_t b038: 1; /**< Block 38 */
			uint32_t b039: 1; /**< Block 39 */
			uint32_t b040: 1; /**< Block 40 */
			uint32_t b041: 1; /**< Block 41 */
			uint32_t b042: 1; /**< Block 42 */
			uint32_t b043: 1; /**< Block 43 */
			uint32_t b044: 1; /**< Block 44 */
			uint32_t b045: 1; /**< Block 45 */
			uint32_t b046: 1; /**< Block 46 */
			uint32_t b047: 1; /**< Block 47 */
			uint32_t b048: 1; /**< Block 48 */
			uint32_t b049: 1; /**< Block 49 */
			uint32_t b050: 1; /**< Block 50 */
			uint32_t b051: 1; /**< Block 51 */
			uint32_t b052: 1; /**< Block 52 */
			uint32_t b053: 1; /**< Block 53 */
			uint32_t b054: 1; /**< Block 54 */
			uint32_t b055: 1; /**< Block 55 */
			uint32_t b056: 1; /**< Block 56 */
			uint32_t b057: 1; /**< Block 57 */
			uint32_t b058: 1; /**< Block 58 */
			uint32_t b059: 1; /**< Block 59 */
			uint32_t b060: 1; /**< Block 60 */
			uint32_t b061: 1; /**< Block 61 */
			uint32_t b062: 1; /**< Block 62 */
			uint32_t b063: 1; /**< Block 63 */
			uint32_t b064: 1; /**< Block 64 */
			uint32_t b065: 1; /**< Block 65 */
			uint32_t b066: 1; /**< Block 66 */
			uint32_t b067: 1; /**< Block 67 */
			uint32_t b068: 1; /**< Block 68 */
			uint32_t b069: 1; /**< Block 69 */
			uint32_t b070: 1; /**< Block 70 */
			uint32_t b071: 1; /**< Block 71 */
			uint32_t b072: 1; /**< Block 72 */
			uint32_t b073: 1; /**< Block 73 */
			uint32_t b074: 1; /**< Block 74 */
			uint32_t b075: 1; /**< Block 75 */
			uint32_t b076: 1; /**< Block 76 */
			uint32_t b077: 1; /**< Block 77 */
			uint32_t b078: 1; /**< Block 78 */
			uint32_t b079: 1; /**< Block 79 */
			uint32_t b080: 1; /**< Block 80 */
			uint32_t b081: 1; /**< Block 81 */
			uint32_t b082: 1; /**< Block 82 */
			uint32_t b083: 1; /**< Block 83 */
			uint32_t b084: 1; /**< Block 84 */
			uint32_t b085: 1; /**< Block 85 */
			uint32_t b086: 1; /**< Block 86 */
			uint32_t b087: 1; /**< Block 87 */
			uint32_t b088: 1; /**< Block 88 */
			uint32_t b089: 1; /**< Block 89 */
			uint32_t b090: 1; /**< Block 90 */
			uint32_t b091: 1; /**< Block 91 */
			uint32_t b092: 1; /**< Block 92 */
			uint32_t b093: 1; /**< Block 93 */
			uint32_t b094: 1; /**< Block 94 */
			uint32_t b095: 1; /**< Block 95 */
			uint32_t b096: 1; /**< Block 96 */
			uint32_t b097: 1; /**< Block 97 */
			uint32_t b098: 1; /**< Block 98 */
			uint32_t b099: 1; /**< Block 99 */
			uint32_t b100: 1; /**< Block 100 */
			uint32_t b101: 1; /**< Block 101 */
			uint32_t b102: 1; /**< Block 102 */
			uint32_t b103: 1; /**< Block 103 */
			uint32_t b104: 1; /**< Block 104 */
			uint32_t b105: 1; /**< Block 105 */
			uint32_t b106: 1; /**< Block 106 */
			uint32_t: 21;
		} BPS_b;
	};
} flash_ra_cf_block_map;

/**
 * @brief Input parameters for @ref FLASH_RA_EX_OP_WRITE_PROTECT operation.
 */
typedef struct flash_ra_ex_write_protect_in {
	/** Bitmask of blocks to enable write protection for. */
	flash_ra_cf_block_map protect_enable;
	/** Bitmask of blocks to disable write protection for. */
	flash_ra_cf_block_map protect_disable;
	/** Bitmask of blocks to permanently enable write protection for. */
	flash_ra_cf_block_map protect_permanent;
} flash_ra_ex_write_protect_in_t;

/**
 * @brief Output parameters for @ref FLASH_RA_EX_OP_WRITE_PROTECT operation.
 *
 * This is populated by the driver to report the current write protection settings.
 */
typedef struct flash_ra_ex_write_protect_out {
	/** Bitmask of blocks that are currently write-protected. */
	flash_ra_cf_block_map protected_enabled;
	/** Bitmask of blocks that are permanently write-protected. */
	flash_ra_cf_block_map protected_premanent;
} flash_ra_ex_write_protect_out_t;

/**
 * @}
 */

#endif /* __ZEPHYR_INCLUDE_DRIVERS_FLASH_RA_FLASH_API_EXTENSIONS_H__ */
