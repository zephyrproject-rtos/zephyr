/*
 * Copyright (c) 2024 Grant Ramsay <grant.ramsay@hotmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_NET_LIB_SSH_SSH_HOST_KEY_H_
#define ZEPHYR_SUBSYS_NET_LIB_SSH_SSH_HOST_KEY_H_

#include "ssh_core.h"
#include "ssh_pkt.h"

#include <zephyr/kernel.h>
#include <zephyr/net/ssh/keygen.h>

int ssh_host_key_write_pub_key(struct ssh_payload *payload, int host_key_index);
int ssh_host_key_write_signature(struct ssh_payload *payload, int host_key_index,
				 enum ssh_host_key_alg host_key_alg,
				 const void *data, uint32_t data_len);

/* Set host_key_alg to NULL for any supported alg */
int ssh_host_key_verify_signature(const enum ssh_host_key_alg *host_key_alg,
				  const struct ssh_string *host_key_blob,
				  const struct ssh_string *signature,
				  const void *data, uint32_t data_len);

#endif
