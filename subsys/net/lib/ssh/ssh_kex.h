/*
 * Copyright (c) 2024 Grant Ramsay <grant.ramsay@hotmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_NET_LIB_SSH_SSH_KEX_H_
#define ZEPHYR_SUBSYS_NET_LIB_SSH_SSH_KEX_H_

#include "ssh_transport.h"

int ssh_kex_send_kexinit(struct ssh_transport *transport);
int ssh_kex_send_newkeys(struct ssh_transport *transport);
int ssh_kex_process_kexinit(struct ssh_transport *transport, struct ssh_payload *rx_pkt);
int ssh_kex_process_newkeys(struct ssh_transport *transport, struct ssh_payload *rx_pkt);
int ssh_kex_process_msg(struct ssh_transport *transport, uint8_t msg_id,
			struct ssh_payload *rx_pkt);

int ssh_kex_gen_exchange_hash(struct ssh_transport *transport,
			      const struct ssh_string *client_ephemeral_key,
			      const struct ssh_string *server_host_key);

#ifdef CONFIG_SSH_CLIENT
int ssh_kex_ecdh_send_kex_ecdh_init(struct ssh_transport *transport);
#endif

int ssh_kex_ecdh_process_msg(struct ssh_transport *transport, uint8_t msg_id,
			     struct ssh_payload *rx_pkt);

#endif
