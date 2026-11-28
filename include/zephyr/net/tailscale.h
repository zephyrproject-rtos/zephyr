/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: FoBE Studio
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for the managed Tailscale virtual network interface.
 * @ingroup tailscale
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_NET_TAILSCALE_H_
#define ZEPHYR_INCLUDE_ZEPHYR_NET_TAILSCALE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/net/tls_credentials.h>

#ifdef __cplusplus
extern "C" {
#endif

struct net_if;

/**
 * @defgroup tailscale Tailscale client
 * @ingroup networking
 * @brief Join a tailnet and manage its virtual network interface.
 * @since 4.6
 * @version 0.1.0
 *
 * Experimental API for managing one Tailscale virtual interface. Applications
 * use standard sockets to send and receive traffic over the interface.
 *
 * @{
 */

/** Maximum length of the node hostname, excluding the NUL terminator. */
#define TAILSCALE_HOSTNAME_MAX      63U
/** Buffer size for a DNS name, including the NUL terminator. */
#define TAILSCALE_DNS_NAME_SIZE     254U
/** Maximum number of local service rules. */
#define TAILSCALE_SERVICE_MAX_RULES 4U

/** Whether to save the node identity across reboots. */
enum tailscale_identity_mode {
	/** Store the node identity in Zephyr Settings. */
	TAILSCALE_IDENTITY_PERSISTENT,
	/** Keep the node identity in memory and request ephemeral registration. */
	TAILSCALE_IDENTITY_EPHEMERAL,
};

/** A local service eligible for Tailnet ingress when allowed by policy. */
struct tailscale_service_rule {
	/** Nonzero local port in host byte order. */
	uint16_t port;
	/** NET_IPPROTO_TCP or NET_IPPROTO_UDP. */
	uint8_t protocol;
};

/** Routing preferences for the node. */
struct tailscale_routes {
	/** Accept subnet routes from the control server. */
	bool accept_routes;
	/** Retain access to the directly connected physical LAN with an exit node. */
	bool allow_lan;
	/** Empty, a stable ID, node IP, DNS name or auto:any. */
	char exit_node[TAILSCALE_DNS_NAME_SIZE];
};

/**
 * @brief Report a confidential enrollment URL during the first start.
 *
 * The callback runs during tailscale_start() in the calling thread.
 *
 * @param[in] url NUL-terminated HTTPS URL valid for the duration of the callback.
 * @param[in] context Application data from the start configuration.
 */
typedef void (*tailscale_login_cb_t)(const char *url, void *context);

/** Configuration for one Tailscale node per image. */
struct tailscale_client_config {
	/** Connected physical interface that carries control and tunnel traffic. */
	struct net_if *underlay;
	/**
	 * NUL-terminated device name with 1 to #TAILSCALE_HOSTNAME_MAX characters.
	 * Use ASCII letters, digits and hyphens.
	 */
	const char *hostname;
	/** Nonnegative tag of a registered certificate authority credential. */
	sec_tag_t ca_tag;
	/**
	 * Deadline for one control operation in milliseconds. Allow for DNS,
	 * two TCP and TLS connection attempts, and application protocol I/O.
	 */
	uint32_t timeout_ms;
	/** Persistence of the local node identity. */
	enum tailscale_identity_mode identity_mode;
	/** Optional NUL-terminated preauthorization key, borrowed during start. */
	const char *auth_key;
	/** Optional callback to receive a URL when the control server requires approval. */
	tailscale_login_cb_t login;
	/** User data for the enrollment callback. */
	void *login_context;
	/** Optional routing preferences, copied during start. */
	const struct tailscale_routes *routes;
	/** Optional local service allowlist, copied during start. */
	const struct tailscale_service_rule *services;
	/** Number of entries in services, up to TAILSCALE_SERVICE_MAX_RULES. */
	size_t service_count;
};

/** Action to take with the identity after withdrawing the interface. */
enum tailscale_stop_mode {
	/** Keep the identity for the next start. */
	TAILSCALE_STOP_KEEP_IDENTITY,
	/** Retire the identity and request removal from the control server. */
	TAILSCALE_STOP_LOGOUT,
};

/**
 * @brief Start the managed virtual interface from thread context.
 *
 * Before calling this function, bring up the underlay with a preferred IPv4
 * address and set trusted UTC. Keep the TLS credential available for the
 * lifetime of the client.
 *
 * After the client initializes, use the same underlay, hostname, CA tag,
 * timeout, and identity mode for every subsequent start until reboot.
 * This applies after tailscale_stop() and when retrying enrollment.
 *
 * Persistent identities use the Zephyr Settings backend. Protect that storage
 * as private key material.
 *
 * @param[in] config Non-NULL configuration. Its strings and rule arrays are borrowed for
 *                   this call only. The underlay and TLS credential remain in use.
 * @param[out] iface Managed interface on success, or NULL to omit the output.
 *                   Wait for the interface to become operational before using it.
 * @retval 0 The background control session was started.
 * @retval -EINPROGRESS User approval is pending. Call again after approval.
 * @retval -ENOENT First enrollment requires a key or approval callback.
 * @retval -ENETDOWN The underlay has no preferred IPv4 address.
 * @retval -ENODATA Trusted UTC has not been set.
 * @retval -EINVAL
 *     - The configuration is invalid.
 *     - A fixed configuration field changed after initialization.
 * @retval -EALREADY
 *     - The client is already starting or running.
 *     - The service list contains duplicate rules.
 * @retval -EBUSY A client operation is already in progress.
 * @return Other negative errno values on failure.
 */
int tailscale_start(const struct tailscale_client_config *config, struct net_if **iface);

/**
 * @brief Replace the running client's routing and service configuration from thread context.
 *
 * The client copies the new preferences and rules during this call.
 *
 * @param[in] routes New preferences, or NULL to restore defaults.
 * @param[in] services Local services eligible for new inbound connections.
 *                     May be NULL when @p service_count is zero.
 * @param[in] service_count Number of entries in @p services, up to
 *                          #TAILSCALE_SERVICE_MAX_RULES. Zero clears the allowlist.
 * @retval 0 The configuration was replaced.
 * @retval -EINVAL The routing preferences or service rules are invalid.
 * @retval -EALREADY The service list contains duplicate rules.
 * @retval -ENETDOWN The managed interface is inactive.
 * @retval -EBUSY A client operation is already in progress.
 * @return Other negative errno values on failure.
 */
int tailscale_configure(const struct tailscale_routes *routes,
			const struct tailscale_service_rule *services, size_t service_count);

/**
 * @brief Withdraw the managed interface and stop control and tunnel traffic from thread context.
 *
 * During logout, local retirement takes effect even if the subsequent remote
 * removal returns an error.
 *
 * @param[in] mode Whether to retain or retire the identity.
 * @retval 0 The requested operation completed.
 * @retval -EINVAL The stop mode is invalid.
 * @retval -EAGAIN The client has not been initialized.
 * @retval -EBUSY A client operation is already in progress.
 * @return Other negative errno values on failure.
 */
int tailscale_stop(enum tailscale_stop_mode mode);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_NET_TAILSCALE_H_ */
