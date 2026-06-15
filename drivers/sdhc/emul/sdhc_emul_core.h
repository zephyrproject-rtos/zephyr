/*
 * Copyright 2026 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SDHC_SDHC_EMUL_CORE_H_
#define ZEPHYR_DRIVERS_SDHC_SDHC_EMUL_CORE_H_

#include <zephyr/drivers/sdhc.h>
#include <zephyr/sd/sd_spec.h>
#include "sdhc_emul_types.h"

void sdhc_emul_core_build_cid(struct sdhc_emul_card *card, int ordinal);
void sdhc_emul_core_build_csd(struct sdhc_emul_card *card);
void sdhc_emul_core_build_ext_csd(struct sdhc_emul_card *card);
void sdhc_emul_core_init_sdio_regs(struct sdhc_emul_card *card);
uint8_t *sdhc_emul_core_block_ptr(struct sdhc_emul_card *card, uint32_t block_num);
int sdhc_emul_core_validate_data(struct sdhc_emul_card *card, struct sdhc_data *data);
int sdhc_emul_core_xfer_size(struct sdhc_emul_card *card, struct sdhc_data *data,
			     size_t *total_size);
int sdhc_emul_core_request(struct sdhc_emul_card *card, struct sdhc_command *cmd,
			   struct sdhc_data *data);

#endif
