/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_SMP_TEST_UTIL_
#define H_SMP_TEST_UTIL_

#include <zephyr/ztest.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zcbor_common.h>
#include <smp_internal.h>

/* SMP header function for generating os_mgmt info command header with sequence number set to 1 */
void smp_make_hdr(struct smp_hdr *rsp_hdr, size_t len, uint8_t version);

/* Function for creating an os_mgmt info command */
bool create_mcumgr_format_packet(zcbor_state_t *zse, const uint8_t *format, uint8_t *buffer,
				 uint8_t *output_buffer, uint16_t *buffer_size, uint8_t version);


#endif
