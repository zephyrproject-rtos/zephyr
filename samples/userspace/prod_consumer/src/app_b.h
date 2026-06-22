/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PROD_CONSUMER_APP_B_H
#define PROD_CONSUMER_APP_B_H

#include <zephyr/kernel.h>
#include <zephyr/app_memory/app_memdomain.h>

void app_b_entry(void *p1, void *p2, void *p3);

extern struct k_mem_partition app_b_partition;
#define APP_B_DATA	K_APP_DMEM(app_b_partition)
#define APP_B_BSS	K_APP_BMEM(app_b_partition)

#endif /* PROD_CONSUMER_APP_B_H */
