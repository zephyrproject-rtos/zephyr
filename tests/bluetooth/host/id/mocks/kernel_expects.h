/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

/*
 *  Validate expected behaviour when k_work_init_delayable() is called
 *
 *  Expected behaviour:
 *   - k_work_init_delayable() to be called once with correct parameters
 */
void expect_single_call_k_work_init_delayable(struct k_work_delayable *dwork);
