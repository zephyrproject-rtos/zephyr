/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_SMP_TEST_UTIL_
#define H_SMP_TEST_UTIL_

#include <zephyr/ztest.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zcbor_common.h>
#include <smp_internal.h>

#ifdef CONFIG_ZCBOR_CANONICAL
#define CBOR_MAP_STATES 3
#else
#define CBOR_MAP_STATES 2
#endif

/* Function for creating an os_mgmt echo command */
bool create_os_mgmt_echo_packet(zcbor_state_t *zse, uint8_t *buffer, uint8_t *output_buffer,
				uint16_t *buffer_size, char *data);

/* Function for creating an os_mgmt echo response */
bool create_os_mgmt_echo_response_packet(zcbor_state_t *zse, uint8_t *buffer,
					 uint8_t *output_buffer, uint16_t *buffer_size,
					 char *data);

/* Function for creating a transport_mgmt connect command */
bool create_transport_mgmt_connect_packet(zcbor_state_t *zse, uint8_t *buffer,
					  uint8_t *output_buffer, uint16_t *buffer_size,
					  uint8_t transport_id);

/* Function for creating a transport_mgmt disconnect command */
bool create_transport_mgmt_disconnect_packet(zcbor_state_t *zse, uint8_t *buffer,
					     uint8_t *output_buffer, uint16_t *buffer_size,
					     uint8_t transport_id, bool all);

/* Function for creating a transport_mgmt status command */
bool create_transport_mgmt_status_packet(zcbor_state_t *zse, uint8_t *buffer,
					 uint8_t *output_buffer, uint16_t *buffer_size);

/* Function for creating a transport_mgmt transport list command */
bool create_transport_mgmt_transports_packet(zcbor_state_t *zse, uint8_t *buffer,
					     uint8_t *output_buffer, uint16_t *buffer_size);

/* Function for creating a transport_mgmt modes of transport command */
bool create_transport_mgmt_modes_packet(zcbor_state_t *zse, uint8_t *buffer,
					uint8_t *output_buffer, uint16_t *buffer_size,
					uint32_t transport_id);

/* Function for creating a transport_mgmt config details of transport command */
bool create_transport_mgmt_config_details_packet(zcbor_state_t *zse, uint8_t *buffer,
						 uint8_t *output_buffer, uint16_t *buffer_size,
						 uint32_t transport_id, uint32_t type);


#endif
