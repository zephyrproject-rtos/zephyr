/*
 * Copyright 2026 Alif Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

/* Setup function declarations */
extern void *sdhc_rw_setup(void);
extern void *sdhc_emmc_setup(void);

ZTEST_SUITE(sdhc_init, NULL, NULL, NULL, NULL, NULL);
ZTEST_SUITE(sdhc_rw, NULL, sdhc_rw_setup, NULL, NULL, NULL);
ZTEST_SUITE(sdhc_emmc, NULL, sdhc_emmc_setup, NULL, NULL, NULL);
ZTEST_SUITE(sdhc_sdio, NULL, NULL, NULL, NULL, NULL);
ZTEST_SUITE(sdhc_faults, NULL, NULL, NULL, NULL, NULL);
