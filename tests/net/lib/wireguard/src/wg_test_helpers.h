/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_NET_LIB_WIREGUARD_WG_TEST_HELPERS_H_
#define ZEPHYR_TESTS_NET_LIB_WIREGUARD_WG_TEST_HELPERS_H_

/**
 * @file
 * @brief Test-only helpers for the WireGuard unit tests.
 *
 * These build synthetic transport sessions and drive the WireGuard receive
 * path directly. They live in the test tree and reach into the library only
 * through functions exported via ZTESTABLE_STATIC (see subsys/net/lib/wireguard).
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/wireguard.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Pure replay-window check exported by the library via ZTESTABLE_STATIC.
 *
 * @param replay_bitmap Sliding replay bitmap (updated in place on success).
 * @param replay_counter Replay algorithm counter (greatest observed seq + 1) (updated on success).
 * @param seq WireGuard transport counter from the packet header.
 *
 * @return true if the counter is new or acceptably out-of-order, false otherwise.
 */
bool wg_replay_check(uint32_t *replay_bitmap, uint64_t *replay_counter, uint64_t seq);

/** Session material for wireguard_test_install_session(). */
struct wireguard_test_session {
	/** AEAD key used to decrypt inbound transport data. */
	uint8_t recv_key[32];
	/** AEAD key used to encrypt outbound transport data (peer side). */
	uint8_t send_key[32];
	/** Local session index (receiver index in inbound packets). */
	uint32_t local_index;
	/** Remote session index. */
	uint32_t remote_index;
};

/** Snapshot of whether prev/current/next keypairs are valid. */
struct wireguard_test_keypair_state {
	bool prev_valid;
	bool current_valid;
	bool next_valid;
};

/**
 * @brief Add a test peer with a fixed interface private key.
 *
 * @param iface_out Optional pointer to receive the peer virtual interface.
 *
 * @return Peer id on success, negative errno otherwise.
 */
int wireguard_test_peer_add(struct net_if **iface_out);

/**
 * @brief Install a synthetic transport session on a test peer.
 *
 * @param peer_id Peer id from wireguard_test_peer_add().
 * @param session Session keys and indices.
 *
 * @return 0 on success, negative errno otherwise.
 */
int wireguard_test_install_session(int peer_id,
				   const struct wireguard_test_session *session);

/**
 * @brief Encrypt transport plaintext with a raw session key.
 *
 * @param key 32-byte ChaCha20-Poly1305 key.
 * @param counter Transport-data counter.
 * @param plaintext Plaintext (may be NULL when @p plaintext_len is 0).
 * @param plaintext_len Plaintext length in bytes.
 * @param ciphertext Output buffer for ciphertext (plaintext + tag).
 * @param ciphertext_size Size of @p ciphertext buffer.
 * @param ciphertext_len Output ciphertext length.
 *
 * @return 0 on success, negative errno otherwise.
 */
int wireguard_test_encrypt(const uint8_t key[32], uint64_t counter,
			   const uint8_t *plaintext, size_t plaintext_len,
			   uint8_t *ciphertext, size_t ciphertext_size,
			   size_t *ciphertext_len);

/**
 * @brief Inject a transport-data message into the receive path.
 *
 * @param peer_id Peer id from wireguard_test_peer_add().
 * @param src Source UDP/IP address of the outer packet.
 * @param counter Transport-data counter.
 * @param ciphertext Encrypted transport payload (including Poly1305 tag).
 * @param ciphertext_len Length of @p ciphertext.
 *
 * @return 0 on success, negative errno otherwise.
 */
int wireguard_test_inject_transport(int peer_id,
				    const struct net_sockaddr *src,
				    uint64_t counter,
				    const uint8_t *ciphertext,
				    size_t ciphertext_len);

/**
 * @brief Read accumulated VPN statistics for a peer interface.
 *
 * @param peer_id Peer id from wireguard_test_peer_add().
 * @param stats Output statistics (unchanged on error).
 *
 * @return 0 on success, negative errno otherwise.
 */
int wireguard_test_get_stats(int peer_id, struct net_stats_vpn *stats);

/** @brief Query whether a peer has received a valid transport message. */
bool wireguard_test_peer_first_valid(int peer_id);

/** @brief Read peer last_rx tick value. */
uint32_t wireguard_test_peer_last_rx(int peer_id);

/** @brief Copy the peer roaming endpoint. */
int wireguard_test_peer_endpoint(int peer_id,
				 struct net_sockaddr_storage *endpoint);

/** @brief Snapshot prev/current/next keypair validity. */
int wireguard_test_peer_keypair_state(int peer_id,
				      struct wireguard_test_keypair_state *state);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_TESTS_NET_LIB_WIREGUARD_WG_TEST_HELPERS_H_ */
