/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ESPI_ESPI_TAF_NXP_H_
#define ZEPHYR_DRIVERS_ESPI_ESPI_TAF_NXP_H_

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct espi_saf_hw_cfg {
	uint8_t _unused;
};

struct espi_saf_flash_cfg {
	uint32_t flashsz;
	uint8_t flags;
};

struct espi_saf_pr {
	uint32_t start;
	uint32_t size;
	uint8_t master_bm_we;
	uint8_t master_bm_rd;
	uint8_t pr_num;
	uint8_t flags;
};

struct espi_saf_protection {
	size_t nregions;
	const struct espi_saf_pr *pregions;
};

struct espi_nxp_taf_req {
	uint32_t addr;
	uint32_t len;
	uint8_t port;
	uint8_t tag;
	uint8_t type;
	bool read_start;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_ESPI_ESPI_TAF_NXP_H_ */
