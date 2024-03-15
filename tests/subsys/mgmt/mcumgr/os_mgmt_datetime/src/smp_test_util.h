/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * Copyright (c) 2023 Jamie M.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_SMP_TEST_UTIL_
#define H_SMP_TEST_UTIL_

#include <zephyr/ztest.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zcbor_common.h>
#include <smp_internal.h>
#include <zephyr/drivers/rtc.h>

/* SMP header function for generating os_mgmt datetime command header with sequence number set
 * to 1
 */
void smp_make_hdr(struct smp_hdr *rsp_hdr, size_t len, bool version2, bool write);

/* Function for creating an os_mgmt datetime get command */
bool create_mcumgr_datetime_get_packet(zcbor_state_t *zse, bool version2, uint8_t *buffer,
				       uint8_t *output_buffer, uint16_t *buffer_size);

/* Function for creating an os_mgmt datetime set command */
bool create_mcumgr_datetime_set_packet_str(zcbor_state_t *zse, bool version2, const char *data,
					   uint8_t *buffer, uint8_t *output_buffer,
					   uint16_t *buffer_size);

bool create_mcumgr_datetime_set_packet(zcbor_state_t *zse, bool version2, struct rtc_time *a_time,
				       uint8_t *buffer, uint8_t *output_buffer,
				       uint16_t *buffer_size);

#endif
