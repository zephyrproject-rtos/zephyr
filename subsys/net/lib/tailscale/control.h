/*
 * SPDX-FileCopyrightText: FoBE Studio
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_NET_TAILSCALE_CONTROL_H_
#define ZEPHYR_NET_TAILSCALE_CONTROL_H_

#include "tailscale_internal.h"
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>

struct tailscale_control_params {
	const uint8_t *machine_private;
	const uint8_t *node_private;
	const uint8_t *disco_private;
	const char *auth_key;
	tailscale_login_cb_t login_required;
	void *login_context;
	struct net_if *underlay;
	const char *hostname;
	struct net_sockaddr_storage local_endpoint;
	sec_tag_t ca_tag;
	uint32_t timeout_ms;
	bool ephemeral;
};

bool tailscale_control_registration_valid(const char *hostname, const char *auth_key,
					uint32_t timeout_ms);
int tailscale_control_enroll(const struct tailscale_control_params *params);
int tailscale_control_logout(const struct tailscale_control_params *params);

/* The keyless watcher borrows the resident identity and hostname until stop. */
int tailscale_control_watch_start(const struct tailscale_control_params *params,
				  struct net_if *lan);
int tailscale_control_watch_stop(void);
void tailscale_control_watch_status(struct tailscale_watch_status *status);

#endif /* ZEPHYR_NET_TAILSCALE_CONTROL_H_ */
