/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_SMP_TEST_UTIL_
#define H_SMP_TEST_UTIL_

#include <zephyr/ztest.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zcbor_common.h>
#include <smp_internal.h>

/* Function for creating an os_mgmt memorypool command */
bool create_os_mgmt_mpstat_packet(zcbor_state_t *zse, uint8_t *buffer, uint8_t *output_buffer,
				  uint16_t *buffer_size);

#endif
