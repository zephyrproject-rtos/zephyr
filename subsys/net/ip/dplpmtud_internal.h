/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief IPv4/6 DPLPMTUD related functions
 */

#ifndef ZEPHYR_SUBSYS_NET_IP_DPLPMTUD_H_
#define ZEPHYR_SUBSYS_NET_IP_DPLPMTUD_H_

#include <zephyr/net/dplpmtud.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum number of retries for a single probe size before clamping search. */
#define NET_DPLPMTUD_MAX_PROBE_RETRIES 3U

/** Per-destination DPLPMTUD state. */
struct net_dplpmtud_entry {
	/** Destination address. */
	struct net_addr dst;

	/** Last time the entry was updated. */
	uint32_t last_update;

	/** Base PLPMTU for this path. */
	uint16_t base_plpmtu;

	/** Largest validated PLPMTU for this path. */
	uint16_t validated_plpmtu;

	/** Maximum candidate PLPMTU currently allowed for this path. */
	uint16_t max_plpmtu;

	/** Lower search bound for the next probe. */
	uint16_t search_low;

	/** Upper search bound for the next probe. */
	uint16_t search_high;

	/** Size of the current probe. */
	uint16_t probe_size;

	/** Number of attempts made for the current probe size. */
	uint8_t probe_count;

	/** Number of black-hole fallbacks recorded for this path. */
	uint8_t blackhole_count;

	/** Current search state. */
	enum net_dplpmtud_state state;

	/** Entry is in use. */
	bool in_use : 1;

	/** A probe is currently in flight. */
	bool probe_in_flight : 1;
};

/**
 * @typedef net_dplpmtud_cb_t
 * @brief Callback used when traversing DPLPMTUD path state.
 *
 * @param entry DPLPMTUD entry
 * @param user_data User specified data
 */
typedef void (*net_dplpmtud_cb_t)(struct net_dplpmtud_entry *entry,
				  void *user_data);

/**
 * @brief Get DPLPMTUD entry for the given destination address.
 *
 * @param dst Destination address
 *
 * @return DPLPMTUD entry if found, NULL otherwise
 */
struct net_dplpmtud_entry *net_dplpmtud_get_entry(const struct net_sockaddr *dst);

/**
 * @brief Get or create DPLPMTUD entry for the given destination address.
 *
 * @param dst Destination address
 *
 * @return DPLPMTUD entry if found or created, NULL otherwise
 */
struct net_dplpmtud_entry *net_dplpmtud_get_or_create_entry(const struct net_sockaddr *dst);

/**
 * @brief Initialize DPLPMTUD entry state.
 *
 * @param entry DPLPMTUD entry
 * @param base_plpmtu Base PLPMTU for this path
 * @param max_plpmtu Maximum probe candidate for this path
 */
void net_dplpmtud_init_entry(struct net_dplpmtud_entry *entry,
			     uint16_t base_plpmtu,
			     uint16_t max_plpmtu);

/**
 * @brief Set base PLPMTU for an entry.
 *
 * @param entry DPLPMTUD entry
 * @param base_plpmtu New base PLPMTU
 */
void net_dplpmtud_set_base_plpmtu(struct net_dplpmtud_entry *entry, uint16_t base_plpmtu);

/**
 * @brief Set maximum PLPMTU candidate for an entry.
 *
 * @param entry DPLPMTUD entry
 * @param max_plpmtu New maximum PLPMTU candidate
 */
void net_dplpmtud_set_max_plpmtu(struct net_dplpmtud_entry *entry, uint16_t max_plpmtu);

/**
 * @brief Get the next probe size for an entry.
 *
 * @param entry DPLPMTUD entry
 *
 * @return Next probe size, or 0 if search is complete
 */
uint16_t net_dplpmtud_get_next_probe_size(struct net_dplpmtud_entry *entry);

/**
 * @brief Mark a probe as in flight.
 *
 * @param entry DPLPMTUD entry
 * @param probe_size Probe size being transmitted
 *
 * @return 0 if success, negative errno otherwise
 */
int net_dplpmtud_start_probe(struct net_dplpmtud_entry *entry, uint16_t probe_size);

/**
 * @brief Mark the current probe as acknowledged.
 *
 * @param entry DPLPMTUD entry
 * @param probe_size Probe size that was acknowledged
 *
 * @return 0 if success, negative errno otherwise
 */
int net_dplpmtud_probe_acked(struct net_dplpmtud_entry *entry, uint16_t probe_size);

/**
 * @brief Mark the current probe as lost.
 *
 * @param entry DPLPMTUD entry
 * @param probe_size Probe size that was lost
 *
 * @return 0 if success, negative errno otherwise
 */
int net_dplpmtud_probe_lost(struct net_dplpmtud_entry *entry, uint16_t probe_size);

/**
 * @brief Record a black-hole fallback for an entry.
 *
 * @param entry DPLPMTUD entry
 */
void net_dplpmtud_note_blackhole(struct net_dplpmtud_entry *entry);

/**
 * @brief Apply a PMTU cache update to the matching DPLPMTUD entry.
 *
 * Called when the shared PMTU destination cache changes, for example after
 * an ICMP PTB. When the new MTU is below the base PLPMTU, the path is
 * treated as a black hole.
 *
 * @param dst Destination address
 * @param pmtu New PMTU from the destination cache
 */
void net_dplpmtud_sync_from_pmtu(const struct net_sockaddr *dst, uint16_t pmtu);

/**
 * @brief Traverse DPLPMTUD path state.
 *
 * @param cb Callback to invoke for each entry
 * @param user_data User specified data
 *
 * @return Number of entries visited, negative errno otherwise
 */
int net_dplpmtud_foreach(net_dplpmtud_cb_t cb, void *user_data);

/** Initialize DPLPMTUD module. */
#if defined(CONFIG_NET_PMTU_DPLPMTUD)
void net_dplpmtud_init(void);
#else
static inline void net_dplpmtud_init(void)
{
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_NET_IP_DPLPMTUD_H_ */
