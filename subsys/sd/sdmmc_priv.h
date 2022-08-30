/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_SUBSYS_SD_SDMMC_PRIV_H_
#define ZEPHYR_SUBSYS_SD_SDMMC_PRIV_H_

#include <zephyr/zephyr.h>
#include <zephyr/sd/sd.h>

int sdmmc_card_init(struct sd_card *card);


#endif /* ZEPHYR_SUBSYS_SD_SDMMC_PRIV_H_ */
