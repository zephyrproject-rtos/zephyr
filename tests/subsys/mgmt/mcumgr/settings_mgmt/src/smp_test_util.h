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

/* Function for creating an settings_mgmt read command */
bool create_settings_mgmt_read_packet(zcbor_state_t *zse, uint8_t *buffer, uint8_t *output_buffer,
				      uint16_t *buffer_size, char *name, uint32_t max_size);

/* Function for creating an settings_mgmt write command */
bool create_settings_mgmt_write_packet(zcbor_state_t *zse, uint8_t *buffer, uint8_t *output_buffer,
				       uint16_t *buffer_size, char *name, const uint8_t *val,
				       size_t val_size);

/* Function for creating an settings_mgmt delete command */
bool create_settings_mgmt_delete_packet(zcbor_state_t *zse, uint8_t *buffer,
					uint8_t *output_buffer, uint16_t *buffer_size, char *name);

/* Function for creating an settings_mgmt commit command */
bool create_settings_mgmt_commit_packet(zcbor_state_t *zse, uint8_t *buffer,
					uint8_t *output_buffer, uint16_t *buffer_size);

/* Function for creating an settings_mgmt load command */
bool create_settings_mgmt_load_packet(zcbor_state_t *zse, uint8_t *buffer, uint8_t *output_buffer,
				      uint16_t *buffer_size);

/* Function for creating an settings_mgmt save command */
bool create_settings_mgmt_save_packet(zcbor_state_t *zse, uint8_t *buffer, uint8_t *output_buffer,
				      uint16_t *buffer_size);

#endif
