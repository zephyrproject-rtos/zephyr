/*
 * Copyright (c) 2024 Grant Ramsay <grant.ramsay@hotmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_NET_LIB_SSH_SSH_CONNECTION_H_
#define ZEPHYR_SUBSYS_NET_LIB_SSH_SSH_CONNECTION_H_

#include "ssh_transport.h"

int ssh_connection_process_msg(struct ssh_transport *transport, uint8_t msg_id,
			       struct ssh_payload *rx_pkt);

#ifdef CONFIG_SSH_CLIENT
int ssh_connection_send_channel_open(struct ssh_transport *transport,
				     const struct ssh_string *channel_type,
				     uint32_t recipient_channel,
				     uint32_t initial_window_size,
				     uint32_t maximum_packet_size);
int ssh_connection_send_channel_request_pseudo_terminal(struct ssh_transport *transport,
							uint32_t recipient_channel,
							bool want_reply);
int ssh_connection_send_channel_request(struct ssh_transport *transport,
					uint32_t recipient_channel,
					const struct ssh_string *channel_type,
					bool want_reply);
#endif

#ifdef CONFIG_SSH_SERVER
int ssh_connection_send_channel_open_confirmation(struct ssh_transport *transport,
						  uint32_t recipient_channel,
						  uint32_t sender_channel,
						  uint32_t initial_window_size,
						  uint32_t maximum_packet_size);
#endif

int ssh_connection_send_channel_result(struct ssh_transport *transport, bool success,
				       uint32_t recipient_channel);

struct ssh_channel *ssh_connection_allocate_channel(struct ssh_transport *transport);
void ssh_connection_free_channel(struct ssh_channel *channel);
struct ssh_channel *ssh_connection_get_channel(struct ssh_transport *transport,
					       uint32_t local_channel);

int ssh_connection_send_channel_data(struct ssh_transport *transport,
				     uint32_t recipient_channel,
				     const void *data, uint32_t data_len);
int ssh_connection_send_channel_extended_data(struct ssh_transport *transport,
					      uint32_t recipient_channel,
					      uint32_t data_type_code, const void *data,
					      uint32_t data_len);

int ssh_connection_send_window_adjust(struct ssh_transport *transport,
				      uint32_t recipient_channel, uint32_t bytes_to_add);

#endif
