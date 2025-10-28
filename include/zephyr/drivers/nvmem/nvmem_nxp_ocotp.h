/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_NVMEM_FUSE_NXP_OCOTP_H_
#define ZEPHYR_INCLUDE_DRIVERS_NVMEM_FUSE_NXP_OCOTP_H_

#include <stdint.h>

/* ROM API table for NXP OCOTP (as exposed by the boot ROM) */
typedef struct {
	uint32_t version;
	void (*init)(uint32_t src_clk_freq);
	int32_t (*deinit)(void);
	int32_t (*efuse_read)(uint32_t addr, uint32_t *data);
	int32_t (*efuse_program)(uint32_t addr, uint32_t data);
} nxp_ocotp_driver_t;

#endif /* ZEPHYR_INCLUDE_DRIVERS_NVMEM_FUSE_NXP_OCOTP_H_*/
