/*
 * Copyright 2026 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SDHC_SDHC_EMUL_CORE_H_
#define ZEPHYR_DRIVERS_SDHC_SDHC_EMUL_CORE_H_

#include <zephyr/drivers/sdhc.h>
#include "sdhc_emul_types.h"

/* eMMC EXT_CSD byte offsets (JEDEC JESD84-B51) */
#define EXT_CSD_S_CMD_SET		0
#define EXT_CSD_BOOT_INFO		13
#define EXT_CSD_PARTITION_SWITCH_TIME	48
#define EXT_CSD_STRUCTURE		192
#define EXT_CSD_CARD_TYPE		196
#define EXT_CSD_BUS_WIDTH		183
#define EXT_CSD_HS_TIMING		185
#define EXT_CSD_REV			192
#define EXT_CSD_PWR_CLASS		187
#define EXT_CSD_SEC_COUNT		212
#define EXT_CSD_S_A_TIMEOUT		217
#define EXT_CSD_REL_WR_SEC_C		221
#define EXT_CSD_HC_WP_GRP_SIZE		221
#define EXT_CSD_ERASE_TIMEOUT_MULT	223
#define EXT_CSD_HC_ERASE_GRP_SIZE	224
#define EXT_CSD_BOOT_MULT		226
#define EXT_CSD_SEC_BAD_BLK_MGMNT	229
#define EXT_CSD_BKOPS_SUPPORT		232
#define EXT_CSD_GENERIC_CMD6_TIME	248
#define EXT_CSD_CACHE_SIZE		249
#define EXT_CSD_PWR_CL_DDR_52_195	253
#define EXT_CSD_PWR_CL_DDR_52_360	254
#define EXT_CSD_PWR_CL_200_195		255
#define EXT_CSD_PWR_CL_200_360		256
#define EXT_CSD_PWR_CL_200_130		257
#define EXT_CSD_STOR_SET		258
#define EXT_CSD_BKOPS_EN		232
#define EXT_CSD_HS400_DRIVING		261
#define EXT_CSD_DRIVING_STRENGTH	264
#define EXT_CSD_CACHE_FLUSH_POLICY	255

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
