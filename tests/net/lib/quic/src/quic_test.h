/*
 * Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DEFAULT_QUIC_PORT 42424

void prepare_quic_socket(int *sock,
			 const struct net_sockaddr *remote_addr,
			 const struct net_sockaddr *local_addr);
void setup_quic_certs(int sock, sec_tag_t sec_tag_list[], size_t sec_tag_list_size);
void setup_alpn(int sock, const char * const alpn_list[], size_t alpn_list_size);
