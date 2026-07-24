/*
 * Copyright 2026 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * SDHC emulator types.
 *
 * Defines shared data structures and enums used by the SDHC emulator framework.
 */

#ifndef ZEPHYR_DRIVERS_SDHC_SDHC_EMUL_TYPES_H_
#define ZEPHYR_DRIVERS_SDHC_SDHC_EMUL_TYPES_H_

#include <zephyr/kernel.h>
#include <zephyr/drivers/sdhc.h>
#include <stdint.h>
#include <stdbool.h>

enum sdhc_emul_state {
	SDHC_EMUL_STATE_IDLE = 0,
	SDHC_EMUL_STATE_READY,
	SDHC_EMUL_STATE_IDENT,
	SDHC_EMUL_STATE_STANDBY,
	SDHC_EMUL_STATE_TRANSFER,
	SDHC_EMUL_STATE_SENDING,
	SDHC_EMUL_STATE_RECEIVING,
	SDHC_EMUL_STATE_PROGRAMMING,
	SDHC_EMUL_STATE_DISCONNECT,
	SDHC_EMUL_STATE_SLEEP,
};

enum sdhc_emul_type {
	SDHC_EMUL_TYPE_SD = 0,
	SDHC_EMUL_TYPE_SDHC,
	SDHC_EMUL_TYPE_SDXC,
	SDHC_EMUL_TYPE_EMMC,
	SDHC_EMUL_TYPE_SDIO,
};

struct sdhc_emul_card {
	enum sdhc_emul_state state;
	enum sdhc_emul_type type;
	uint32_t rca;
	uint32_t ocr;
	uint8_t cid[16];
	uint8_t csd[16];
	uint8_t *storage;
	size_t n_blocks;
	uint32_t block_size;
	uint32_t erase_start;
	uint32_t erase_end;
	/* eMMC fields */
	uint8_t ext_csd[512];
	uint8_t bus_width;
	bool hs200_mode;
	bool hs400_mode;
	/* SDIO fields */
	uint8_t sdio_fn_count;
	uint8_t cccr[256];
	uint8_t fbr[8][256];
	uint8_t sdio_fn_buf[8][4096];
	/* Fault injection */
	uint8_t inject_cmd;
	bool card_present;
	/* Interrupt */
	sdhc_interrupt_cb_t irq_cb;
	void *irq_user_data;
	int irq_sources;
	/* ACMD state */
	bool next_is_acmd;
	/* Multi-block state */
	uint32_t multi_block_count;
};

struct sdhc_emul_cfg {
	enum sdhc_emul_type card_type;
	uint32_t n_blocks;
	uint32_t block_size;
	uint8_t inject_cmd;
	uint8_t sdio_fn_cnt;
	bool card_present;
	uint32_t host_ocr;
};

struct sdhc_emul_data {
	struct sdhc_emul_card card;
	struct k_mutex lock;
};

#endif
