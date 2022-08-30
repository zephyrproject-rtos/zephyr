/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sdhc.h>
#include <zephyr/logging/log.h>
#include <zephyr/sd/mmc.h>
#include <zephyr/sd/sd.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/zephyr.h>

#include "sd_ops.h"
#include "sd_utils.h"

inline int mmc_write_blocks(struct sd_card *card, const uint8_t *wbuf, uint32_t start_block,
			    uint32_t num_blocks)
{
	return card_write_blocks(card, wbuf, start_block, num_blocks);
}

inline int mmc_read_blocks(struct sd_card *card, uint8_t *rbuf, uint32_t start_block,
			   uint32_t num_blocks)
{
	return card_read_blocks(card, rbuf, start_block, num_blocks);
}

inline int mmc_ioctl(struct sd_card *card, uint8_t cmd, void *buf)
{
	return card_ioctl(card, cmd, buf);
}

/*
 * Initialize MMC card for use with subsystem
 */
int mmc_card_init(struct sd_card *card)
{
	return -ENOTSUP;
}
