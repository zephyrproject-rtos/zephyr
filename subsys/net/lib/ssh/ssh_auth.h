/*
 * Copyright (c) 2024 Grant Ramsay <grant.ramsay@hotmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_NET_LIB_SSH_SSH_AUTH_H_
#define ZEPHYR_SUBSYS_NET_LIB_SSH_SSH_AUTH_H_

#include "ssh_transport.h"

#ifdef CONFIG_SSH_CLIENT
int ssh_auth_send_userauth_request_none(struct ssh_transport *transport,
					const char *user_name);
int ssh_auth_send_userauth_request_password(struct ssh_transport *transport,
					    const char *user_name,
					    const char *password);
#endif

int ssh_auth_process_msg(struct ssh_transport *transport,
			 uint8_t msg_id, struct ssh_payload *rx_pkt);

#endif
