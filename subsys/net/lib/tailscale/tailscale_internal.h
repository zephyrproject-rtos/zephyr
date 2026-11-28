/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: FoBE Studio
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_NET_TAILSCALE_INTERNAL_H_
#define ZEPHYR_NET_TAILSCALE_INTERNAL_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>

#define TAILSCALE_PEER_STABLE_ID_SIZE 32
#define TAILSCALE_DNS_NAME_SIZE 254U

/** @name Control stream
 * @{
 */

/** @brief Events emitted synchronously by the stream owner. */
enum tailscale_control_stream_event {
	TAILSCALE_CONTROL_STREAM_OPENING,
	TAILSCALE_CONTROL_STREAM_RECEIVING,
	TAILSCALE_CONTROL_STREAM_CLOSED,
};

/** @brief Stream state owned by one worker and cancellable by another thread. */
struct tailscale_control_stream {
	atomic_t cancelled;
	struct k_sem wake;
	uint32_t generation;
};

/** @brief Storage and timing configuration for one worker. */
struct tailscale_control_stream_config {
	/** Exclusive scratch for one decoded message. */
	uint8_t *frame;
	size_t capacity;
	/** Maximum gap between complete frames, including keepalives. */
	uint32_t idle_ms;
	uint32_t retry_min_ms;
	uint32_t retry_max_ms;
};

/** @brief Adapter to an authenticated, uncompressed HTTP response body. */
struct tailscale_control_stream_ops {
	/**
	 * Open a fresh map response without a resume token. Verify its status
	 * before reading. Bound setup and check cancellation during blocking I/O.
	 */
	int (*open)(void *user_data);
	/**
	 * Return zero and a borrowed body chunk, or a negative errno.
	 * A zero-length chunk signals orderly EOF. The bytes may be overwritten
	 * on the next read. The chunk must use storage distinct from the decoded
	 * frame buffer. Honor the absolute uptime deadline and check cancellation
	 * during blocking I/O. This callback must make progress or wait for I/O.
	 */
	int (*read)(int64_t deadline, const uint8_t **data, size_t *len, void *user_data);
	/** Close resources even when open returned an error. */
	void (*close)(void *user_data);
	/**
	 * Apply one complete map response, including keepalives. A new generation
	 * needs a full map before any delta. The mutable scratch belongs to the
	 * stream and is borrowed until return. Return zero to continue.
	 */
	int (*map)(uint32_t generation, uint8_t *json, size_t len, void *user_data);
	/**
	 * Optional notification. CLOSED follows transport cleanup and supplies the
	 * session's errno. Invalidate that generation before another opens.
	 */
	void (*event)(uint32_t generation, enum tailscale_control_stream_event event, int reason,
		      void *user_data);
};

/**
 * @brief Initialize stream storage once before starting a worker.
 * @param stream Instance with storage kept alive until the worker has returned.
 */
void tailscale_control_stream_init(struct tailscale_control_stream *stream);

/**
 * @brief Clear cancellation after the previous worker has returned. Call before
 * publishing a new worker. The generation counter is retained.
 * @param stream Initialized stream instance.
 */
void tailscale_control_stream_prepare(struct tailscale_control_stream *stream);

/**
 * @brief Request shutdown and wake a worker in reconnect backoff.
 * @param stream Initialized stream instance.
 */
void tailscale_control_stream_cancel(struct tailscale_control_stream *stream);

/**
 * @brief Check whether the worker has been asked to stop.
 * @param stream Initialized stream instance.
 * @return True if cancellation was requested.
 */
bool tailscale_control_stream_cancelled(const struct tailscale_control_stream *stream);

/**
 * @brief Run on a single owner thread after initialization. All callbacks are
 * sequential on this thread. The caller keeps config, ops and user_data alive
 * until this returns, and joins the thread before reusing its storage.
 *
 * Transport/decoding failures reopen a fresh generation after bounded backoff.
 * An open error of -EACCES, -EINVAL, -EINPROGRESS, or -ENOTSUP ends the worker.
 *
 * @param stream Initialized stream instance.
 * @param config Worker storage and timing configuration.
 * @param ops Transport and map consumer callbacks.
 * @param user_data Context passed to callbacks.
 * @retval 0 Requested shutdown.
 * @retval -EINVAL Invalid configuration or transport setup.
 * @retval -EACCES Transport authorization failed.
 * @retval -EINPROGRESS Transport requires external approval.
 * @retval -ENOTSUP Transport setup is unsupported.
 */
int tailscale_control_stream_run(struct tailscale_control_stream *stream,
				 const struct tailscale_control_stream_config *config,
				 const struct tailscale_control_stream_ops *ops, void *user_data);

/** @} */

/** @name Peer inventory
 * @{
 */

#ifdef CONFIG_TAILSCALE_PEER_INVENTORY_MAX_PEERS
#define TAILSCALE_PEER_INVENTORY_CAPACITY CONFIG_TAILSCALE_PEER_INVENTORY_MAX_PEERS
#else
#define TAILSCALE_PEER_INVENTORY_CAPACITY 1
#endif
#define TAILSCALE_PEER_ADDRESS_CAPACITY     4
#define TAILSCALE_PEER_PREFIX_CAPACITY      8
#define TAILSCALE_PEER_ENDPOINT_CAPACITY   16
#define TAILSCALE_PEER_PUBLIC_KEY_SIZE     32
#define TAILSCALE_PEER_KEY_EXPIRY_SIZE     40

/** Peer reachability reported by the control server. */
enum tailscale_peer_online {
	TAILSCALE_PEER_ONLINE_UNKNOWN,
	TAILSCALE_PEER_OFFLINE,
	TAILSCALE_PEER_ONLINE,
};

/** An IP prefix with address bytes in network order. */
struct tailscale_peer_prefix {
	uint8_t address[16];
	uint8_t family;
	uint8_t length;
};

/** A physical IP endpoint with address bytes in network order and host-order port. */
struct tailscale_peer_endpoint {
	uint8_t address[16];
	uint16_t port;
	uint8_t family;
};

/** Control plane peer data. Consumers still need to verify and install policy. */
struct tailscale_peer {
	uint64_t id;
#ifdef CONFIG_TAILSCALE_POLICY
	/** UTC milliseconds since the Unix epoch, rounded down, when key_expires is true. */
	int64_t key_expiry_ms;
#endif
	uint8_t key[TAILSCALE_PEER_PUBLIC_KEY_SIZE];
	uint8_t disco_key[TAILSCALE_PEER_PUBLIC_KEY_SIZE];
	char stable_id[TAILSCALE_PEER_STABLE_ID_SIZE];
	char key_expiry[TAILSCALE_PEER_KEY_EXPIRY_SIZE];
	struct tailscale_peer_prefix addresses[TAILSCALE_PEER_ADDRESS_CAPACITY];
	struct tailscale_peer_prefix allowed_ips[TAILSCALE_PEER_PREFIX_CAPACITY];
	struct tailscale_peer_endpoint endpoints[TAILSCALE_PEER_ENDPOINT_CAPACITY];
	uint16_t derp_region;
	uint8_t address_count;
	uint8_t allowed_ip_count;
	uint8_t endpoint_count;
	uint8_t online;
	bool expired;
	bool unsigned_peer_api_only;
	bool key_signature_present;
#ifdef CONFIG_TAILSCALE_POLICY
#ifdef CONFIG_TAILSCALE_ROUTE_POLICY
	/** Bits identifying AllowedIPs designated as primary by the controller. */
	uint8_t primary_routes;
	/** Control permits automatic selection of this exit. */
	bool suggest_exit;
#endif
	/** The authenticated policy map supplies a finite credential expiry. */
	bool key_expires;
	/** Lowercase, fully qualified name without the trailing dot, or empty. */
	char dns_name[TAILSCALE_DNS_NAME_SIZE];
#endif
};

/** Publication metadata. An invalid publication contains zero peers. */
struct tailscale_peer_inventory_summary {
	uint64_t revision;
	uint32_t generation;
	size_t peer_count;
	bool valid;
};

/** The peer inventory effect of an accepted control frame. */
enum tailscale_peer_inventory_frame {
	TAILSCALE_PEER_INVENTORY_KEEPALIVE,
	TAILSCALE_PEER_INVENTORY_FULL,
	TAILSCALE_PEER_INVENTORY_UPDATE,
	TAILSCALE_PEER_INVENTORY_OTHER,
};

#ifdef CONFIG_TAILSCALE_DERP_MAP
/** Address and TLS identity of a relay from authenticated control. */
struct tailscale_derp_node {
	char host[TAILSCALE_DNS_NAME_SIZE];
	char cert_name[TAILSCALE_DNS_NAME_SIZE];
	char ipv4[16];
	char ipv6[46];
	uint16_t region;
	uint16_t port;
	/** Zero when the map disables STUN on this node. */
	uint16_t stun_port;
	bool no_home;
};

/** Relay nodes in control preference order within each region. */
struct tailscale_derp_map {
	struct tailscale_derp_node nodes[CONFIG_TAILSCALE_DERP_MAX_NODES];
	size_t count;
};
#endif

/** Storage for one bounded peer snapshot. */
struct tailscale_peer_snapshot {
#ifdef CONFIG_TAILSCALE_DERP_MAP
	struct tailscale_derp_map derp;
#endif
	struct tailscale_peer peers[TAILSCALE_PEER_INVENTORY_CAPACITY];
	size_t count;
#ifdef CONFIG_TAILSCALE_POLICY
	struct tailscale_peer self;
	/** UTC at an uptime of zero, in milliseconds, when have_control_time is true. */
	int64_t control_epoch_at_boot_ms;
	bool have_self;
	bool have_control_time;
#endif
};

/**
 * @brief Caller-owned storage. Initialize before sharing and use the accessors.
 *
 * The backing arrays are private to the implementation. Allocate the inventory
 * in static or persistent application storage.
 */
struct tailscale_peer_inventory {
	struct k_mutex lock;
	struct tailscale_peer_snapshot banks[2];
	struct tailscale_peer_inventory_summary summary;
	uint64_t sequence;
	uint8_t published;
	bool have_generation;
	bool failed;
};

/** Initialize an inventory before first use. */
void tailscale_peer_inventory_init(struct tailscale_peer_inventory *inventory);

/**
 * @brief Apply a complete control response in thread context.
 *
 * The caller supplies verified HTTP body frames in order on a generation whose
 * number advances on reconnect. A new generation must supply a full response
 * before deltas can publish anything. The inventory does not retain the buffer.
 * A failed application invalidates the current generation. The owner must
 * replace that stream. If frame is non-NULL, it is filled on success.
 *
 * @return 0 on acceptance or a negative errno on failure.
 */
int tailscale_peer_inventory_apply(struct tailscale_peer_inventory *inventory,
				   uint32_t generation, uint8_t *json, size_t len,
				   enum tailscale_peer_inventory_frame *frame);

/** Invalidate a lost control generation. Obsolete generations are ignored. */
void tailscale_peer_inventory_session_lost(struct tailscale_peer_inventory *inventory,
					  uint32_t generation);

/** Copy publication metadata under the inventory lock. */
int tailscale_peer_inventory_get_summary(struct tailscale_peer_inventory *inventory,
					 struct tailscale_peer_inventory_summary *summary);

/** Copy a peer by ID. Return -ENODATA for an invalid inventory or -ENOENT for no match. */
int tailscale_peer_inventory_get_peer(struct tailscale_peer_inventory *inventory, uint64_t id,
				      struct tailscale_peer *peer);

/**
 * @brief Copy a peer by index in a previously read publication.
 *
 * Return -ESTALE if the revision changed and -ENOENT if the index is out of range.
 */
int tailscale_peer_inventory_get_peer_at(struct tailscale_peer_inventory *inventory,
					 uint64_t revision, size_t index,
					 struct tailscale_peer *peer);

/** @} */

/** @name Forwarding and DNS policy
 * @{
 */

#define TAILSCALE_PACKET_FILTER_MAX_RULES 8U
#define TAILSCALE_PACKET_FILTER_MAX_SOURCES 8U
#define TAILSCALE_PACKET_FILTER_MAX_DESTINATIONS 8U
#define TAILSCALE_PACKET_FILTER_MAX_CHUNKS 8U
#define TAILSCALE_PACKET_FILTER_MAX_NAME_BYTES 32U
#define TAILSCALE_DNS_MAX_RESOLVERS 4U
#define TAILSCALE_DNS_MAX_SEARCH_DOMAINS 8U
/* A /10 IPv4 allocation can supply 64 separate reverse DNS zones. */
#define TAILSCALE_DNS_MAX_ROUTES 80U

/** Source of the effective DNS selection for an absolute name. */
enum tailscale_dns_source {
	TAILSCALE_DNS_NONE,
	TAILSCALE_DNS_DEFAULT,
	TAILSCALE_DNS_SPLIT,
	TAILSCALE_DNS_LOCAL,
};

/** A classic DNS server with network-order address bytes and host-order port. */
struct tailscale_dns_server {
	uint8_t address[16];
	uint16_t port;
	uint8_t family;
};

/** One DNS choice and the policy publication from which it was selected. */
struct tailscale_dns_selection {
	uint64_t revision;
	uint32_t generation;
	enum tailscale_dns_source source;
	struct tailscale_dns_server servers[TAILSCALE_DNS_MAX_RESOLVERS];
	uint8_t server_count;
};

/** Local MagicDNS A or AAAA answer from one installed policy publication. */
struct tailscale_dns_answer {
	/** ID of the node whose authorized name was resolved. */
	uint64_t node_id;
	uint64_t revision;
	uint32_t generation;
	struct tailscale_peer_prefix addresses[TAILSCALE_PEER_ADDRESS_CAPACITY];
	uint8_t address_count;
	bool self;
};

/** @cond INTERNAL_HIDDEN */
struct tailscale_filter_address {
	uint8_t first[16];
	uint8_t last[16];
	uint8_t family;
};

struct tailscale_packet_filter_dst {
	struct tailscale_filter_address address;
	uint16_t first_port;
	uint16_t last_port;
};

struct tailscale_packet_filter_rule {
	struct tailscale_filter_address sources[TAILSCALE_PACKET_FILTER_MAX_SOURCES];
	struct tailscale_packet_filter_dst destinations[TAILSCALE_PACKET_FILTER_MAX_DESTINATIONS];
	uint8_t source_count;
	uint8_t destination_count;
	uint8_t protocols;
};

struct tailscale_packet_filter_name {
	char bytes[TAILSCALE_PACKET_FILTER_MAX_NAME_BYTES];
	uint8_t length;
};

struct tailscale_packet_filter_chunk {
	struct tailscale_packet_filter_name name;
	uint8_t rule_count;
};

/* Names and rule counts identify contiguous ranges in the compiled filter. */
struct tailscale_packet_filter_directory {
	struct tailscale_packet_filter_chunk chunks[TAILSCALE_PACKET_FILTER_MAX_CHUNKS];
	uint8_t count;
};

struct tailscale_dns_servers {
	struct tailscale_dns_server entries[TAILSCALE_DNS_MAX_RESOLVERS];
	uint8_t count;
};

struct tailscale_dns_route {
	char suffix[TAILSCALE_DNS_NAME_SIZE];
	struct tailscale_dns_servers servers;
};

struct tailscale_dns_config {
	struct tailscale_dns_servers defaults;
	char search[TAILSCALE_DNS_MAX_SEARCH_DOMAINS][TAILSCALE_DNS_NAME_SIZE];
	uint8_t search_count;
	struct tailscale_dns_route routes[TAILSCALE_DNS_MAX_ROUTES];
	uint8_t route_count;
	bool proxied;
};
/** @endcond */

/** Compiled ingress filter. Use tailscale_packet_filter_accept() to evaluate it. */
struct tailscale_packet_filter {
	struct tailscale_packet_filter_rule rules[TAILSCALE_PACKET_FILTER_MAX_RULES];
	bool reserve_ssh;
	uint8_t count;
};

/** @brief Caller-owned persistent storage. Initialize before sharing. */
struct tailscale_policy {
	struct k_mutex lock;
	struct tailscale_peer_snapshot peers[2];
	struct tailscale_packet_filter filters[2];
	struct tailscale_packet_filter_directory filter_directories[2];
	struct tailscale_dns_config dns[2];
	struct tailscale_peer_inventory_summary summary;
	uint64_t sequence;
	int64_t control_epoch_at_boot_ms;
	uint32_t stream_generation;
	uint8_t published;
	bool have_stream;
	bool have_control_time;
	bool failed;
};

/**
 * @brief Install a complete candidate in thread context.
 *
 * Return zero only after publication and after retiring all readers of the old
 * generation. The installed peers and filter remain immutable until the next
 * successful installation. On a negative errno, retain no candidate pointers.
 * Ordinary errors must leave the old installation intact. The callback must
 * not call back into the policy instance.
 * Return -EKEYREJECTED to report an authenticated loss of local authority.
 * The verified ControlTime is retained when the callback reports this loss.
 *
 * @param peers Candidate peer snapshot, including any expired peer records.
 * @param filter Compiled ingress filter, including deny-all policies.
 * @param context Application context passed to tailscale_policy_apply().
 * @return Zero on successful installation or a negative errno.
 */
typedef int (*tailscale_policy_install_t)(const struct tailscale_peer_snapshot *peers,
					 const struct tailscale_packet_filter *filter,
					 void *context);

/**
 * @brief Initialize a policy with no installed generation.
 * @param policy Caller-owned policy. Any installed filter must be retired first.
 */
void tailscale_policy_init(struct tailscale_policy *policy);

/**
 * @brief Stage and apply an authenticated control frame.
 *
 * Call in stream order, with an advancing generation on reconnect. Rejection
 * preserves the installed snapshot and revision. Replace the rejected stream
 * with a fresh generation and full map. A new stream with no filter denies all
 * ingress. In the same stream, omitted or null filter fields preserve their
 * chunks. A non-null PacketFilter replaces the "base" chunk before PacketFilters
 * is applied. Each named array replaces its chunk, null deletes that chunk,
 * and "*": null clears all chunks before any other named changes. The "*"
 * key is ignored with an array value. Retained rules are merged in name order.
 * A frame can carry up to twice the chunk limit in named changes plus the
 * reserved reset key. Names are bytewise case-sensitive, with the name length
 * limit applied to their decoded UTF-8.
 * Source and destination selectors accept addresses, prefixes, numeric IP
 * ranges and the wildcard. An omitted or empty IPProto selects the TCP and
 * UDP subset of the control default. Unsupported explicit protocols reject
 * the candidate. When an SSH policy is present the filter reserves TCP/22.
 * DNSConfig is inherited within the same stream when omitted or null. An
 * empty object clears DNS, and a new stream starts without inherited DNS.
 * Authenticated ControlTime establishes an estimate of UTC from system uptime
 * that can advance across reconnects to the same authority. A time-only frame
 * can call the installer to re-evaluate expiration without changing the map
 * revision. Reinitialize the policy when changing control authorities.
 *
 * @param policy Initialized policy.
 * @param generation Authenticated stream generation.
 * @param json Complete uncompressed control response body.
 * @param len Size of the body in bytes.
 * @param install Application installer.
 * @param context Context for the installer.
 * @param frame Optional effect of a successful frame.
 * @retval 0 The frame was accepted.
 * @retval -EINVAL Invalid policy or installer.
 * @retval -ESTALE The generation is obsolete or rejected.
 * @retval -EILSEQ A required sequence number is missing or out of order.
 * @retval -EBADMSG The frame is malformed or inconsistent.
 * @retval -E2BIG A bounded map or rule limit was exceeded.
 * @retval -ENOTSUP A required control or filter feature is unsupported.
 * @retval -EKEYREJECTED A credential or trust requirement prevents authorization.
 * @return Other negative errno from the installer.
 */
int tailscale_policy_apply(struct tailscale_policy *policy, uint32_t generation, uint8_t *json,
			   size_t len, tailscale_policy_install_t install, void *context,
			   enum tailscale_peer_inventory_frame *frame);

/**
 * @brief Read publication metadata for the installed policy.
 * @param policy Initialized policy.
 * @param summary Output summary.
 * @retval 0 The summary was copied.
 * @retval -EINVAL Invalid policy or summary.
 */
int tailscale_policy_get_summary(struct tailscale_policy *policy,
				struct tailscale_peer_inventory_summary *summary);

/* Classify a query using the last committed DNS policy, even during an outage. */
int tailscale_policy_dns_authority(struct tailscale_policy *policy, const char *name, bool ready);

/**
 * @brief Select DNS from the installed policy for an opted-in caller.
 *
 * DNS suffixes match case-insensitively at label boundaries, with the longest
 * match taking precedence. TAILSCALE_DNS_LOCAL requires local resolution and
 * carries no upstream servers. TAILSCALE_DNS_NONE has no configured match.
 * The function performs no query and does not change system DNS or routing.
 * The caller owns transport choice and must discard results whose policy
 * generation or revision has since changed.
 *
 * @param policy Initialized policy.
 * @param name Absolute ASCII DNS name, with optional trailing dot.
 * @param selection Output selection, copied together with its revision.
 * @retval 0 The selection was copied.
 * @retval -EINVAL Invalid argument or name.
 * @retval -E2BIG The name exceeds the DNS wire length limit.
 * @retval -ENODATA No policy has been installed.
 */
int tailscale_policy_select_dns(struct tailscale_policy *policy, const char *name,
			       struct tailscale_dns_selection *selection);

/**
 * @brief Copy one configured search domain in publication order.
 *
 * @param policy Initialized policy.
 * @param revision Required DNS policy revision.
 * @param index Zero-based search domain index.
 * @param domain Output buffer for the normalized domain without a trailing dot.
 * @retval 0 The domain was copied.
 * @retval -EINVAL Invalid argument.
 * @retval -ENODATA No policy has been installed.
 * @retval -ESTALE The publication has changed.
 * @retval -ENOENT The index is past the end of the search list.
 */
int tailscale_policy_dns_search_domain(struct tailscale_policy *policy, uint64_t revision,
				      size_t index, char domain[TAILSCALE_DNS_NAME_SIZE]);

/**
 * @brief Look up a name in the installed MagicDNS node map.
 *
 * Only authenticated self and peer host addresses of the requested family are
 * returned. An expired match is ineligible, and duplicate node names are
 * ambiguous. This lookup sends no DNS request. The caller must enforce any
 * additional policy lease or route requirements imposed by its installer.
 *
 * @param policy Initialized policy.
 * @param name ASCII name. A single unqualified label also tries the installed
 *             search domains in order. A trailing dot requests an exact lookup.
 * @param family NET_AF_INET for A or NET_AF_INET6 for AAAA.
 * @param answer Output answer from the policy revision selected by the lookup.
 * @retval 0 An address was found.
 * @retval -EINVAL Invalid argument or name.
 * @retval -E2BIG The name exceeds the DNS wire length limit.
 * @retval -ENODATA No policy has been installed.
 * @retval -ENOTSUP MagicDNS is disabled.
 * @retval -ENOENT The name or address family has no eligible answer.
 * @retval -EADDRINUSE The name is shared by multiple eligible nodes.
 * @retval -EKEYREJECTED Local credentials have expired.
 */
int tailscale_policy_resolve_magic_dns(struct tailscale_policy *policy, const char *name,
				      uint8_t family, struct tailscale_dns_answer *answer);

/** @brief Validated unfragmented TCP or UDP tuple, borrowing packet addresses. */
struct tailscale_filter_tuple {
	/** Source address within the packet. */
	const uint8_t *source;
	/** Destination address within the packet. */
	const uint8_t *destination;
	/** Source port in host byte order. */
	uint16_t source_port;
	/** Destination port in host byte order. */
	uint16_t destination_port;
	/** NET_AF_INET or NET_AF_INET6. */
	uint8_t family;
	/** NET_IPPROTO_TCP or NET_IPPROTO_UDP. */
	uint8_t protocol;
	/** TCP flags byte, or zero for UDP. */
	uint8_t tcp_flags;
};

/**
 * @brief Extract the transport tuple of a supported IP packet.
 *
 * Fragments, IPv6 extension headers and source-routed IPv4 packets are rejected.
 *
 * @param ip Packet beginning with the IP header.
 * @param len Available bytes, including any trailing WireGuard padding.
 * @param tuple Output with addresses valid for the lifetime of @p ip.
 * @return Whether a supported tuple was decoded.
 */
bool tailscale_packet_filter_parse(const uint8_t *ip, size_t len,
				  struct tailscale_filter_tuple *tuple);

/**
 * @brief Match a validated tuple against the installed filter.
 *
 * The caller must verify authenticated source ownership and allowed local destinations.
 *
 * @param filter Immutable compiled filter. NULL denies all.
 * @param tuple Tuple decoded by tailscale_packet_filter_parse().
 * @return Whether the filter authorizes the tuple.
 */
bool tailscale_packet_filter_match(const struct tailscale_packet_filter *filter,
				   const struct tailscale_filter_tuple *tuple);

/**
 * @brief Match a validated peer's raw IP packet against the installed filter.
 *
 * The caller must verify authenticated source ownership and allowed local
 * destinations. Every TCP and UDP packet, including return traffic, requires
 * a matching rule. Fragments and IPv6 extension headers are denied.
 *
 * @param filter Immutable compiled filter. NULL denies all.
 * @param ip Packet beginning with the IP header.
 * @param len Available bytes, including any trailing WireGuard padding.
 * @retval true The filter authorizes the tuple.
 * @retval false The packet is unsupported, malformed or unauthorized.
 */
bool tailscale_packet_filter_accept(const struct tailscale_packet_filter *filter, const uint8_t *ip,
				   size_t len);

/** @} */

#endif /* ZEPHYR_NET_TAILSCALE_INTERNAL_H_ */
