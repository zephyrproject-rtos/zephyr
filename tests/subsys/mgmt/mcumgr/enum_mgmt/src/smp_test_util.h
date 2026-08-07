/*
 * Copyright (c) 2023-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_SMP_TEST_UTIL_
#define H_SMP_TEST_UTIL_

#include <zephyr/ztest.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zcbor_common.h>
#include <smp_internal.h>

/* Function for creating an enum_mgmt count command */
bool create_enum_mgmt_count_packet(zcbor_state_t *zse, uint8_t *buffer, uint8_t *output_buffer,
				   uint16_t *buffer_size);

/* Function for creating an enum_mgmt list command */
bool create_enum_mgmt_list_packet(zcbor_state_t *zse, uint8_t *buffer, uint8_t *output_buffer,
				  uint16_t *buffer_size);

/* Function for creating an enum_mgmt single command */
bool create_enum_mgmt_single_packet(zcbor_state_t *zse, uint8_t *buffer, uint8_t *output_buffer,
				    uint16_t *buffer_size, uint32_t index);

/* Function for creating an enum_mgmt details command */
bool create_enum_mgmt_details_packet(zcbor_state_t *zse, uint8_t *buffer, uint8_t *output_buffer,
				     uint16_t *buffer_size, uint16_t *groups, uint8_t groups_size);

#endif
