/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/base64.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_stats.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/virtual.h>
#include <zephyr/net/virtual_mgmt.h>
#include <zephyr/net/wireguard.h>

#include <psa/crypto.h>

/* Internal library types and constants (struct wg_peer/wg_keypair,
 * msg_transport_data, REJECT_AFTER_TIME, WG_AUTHTAG_LEN, ...).
 */
#include "wg_internal.h"

#include "wg_test_helpers.h"

/* Library internals exported for tests via ZTESTABLE_STATIC. */
struct wg_peer *peer_lookup_by_id(int id);
void keypair_destroy(struct wg_keypair *keypair);
int wg_psa_import_chacha_key(psa_key_id_t *key_id, const uint8_t *key_data,
			     psa_key_usage_t usage);
int wg_psa_aead_encrypt(uint8_t *dst, const uint8_t *src, size_t src_len,
			const uint8_t *ad, size_t ad_len, uint64_t nonce,
			const uint8_t *key);
int wg_process_data_message(struct wg_iface_context *ctx, struct wg_peer *peer,
			    struct msg_transport_data *data_hdr, struct net_pkt *pkt,
			    size_t ip_udp_hdr_len, struct net_sockaddr *addr);

/* Preallocated peer pool / active list, exported via ZTESTABLE_STATIC. */
extern sys_slist_t peer_list;
extern sys_slist_t active_peers;

static void find_wg_iface(struct net_if *iface, void *user_data)
{
	struct net_if **found = user_data;

	if (net_if_l2(iface) != &NET_L2_GET_NAME(VIRTUAL)) {
		return;
	}

	if (net_virtual_get_iface_capabilities(iface) != VIRTUAL_INTERFACE_VPN) {
		return;
	}

	if (*found == NULL) {
		*found = iface;
	}
}

int wireguard_test_peer_add(struct net_if **iface_out)
{
	static const char *iface_privkey = "lmAIbJR8PQOpgJxmfOydBiDbexTMEKsjglZ1sj3kIjs=";
	static int test_peer_id;
	struct virtual_interface_req_params params = { 0 };
	uint8_t private_key[WG_PRIVATE_KEY_LEN];
	struct net_if *iface = NULL;
	struct wg_peer *peer;
	sys_snode_t *node;
	size_t olen;
	int ret;

	net_if_foreach(find_wg_iface, &iface);
	if (iface == NULL) {
		return -ENODEV;
	}

	ret = base64_decode(private_key, sizeof(private_key), &olen, iface_privkey,
			    strlen(iface_privkey));
	if (ret < 0 || olen != sizeof(private_key)) {
		return -EINVAL;
	}

	params.private_key.data = private_key;
	params.private_key.len = sizeof(private_key);

	ret = net_mgmt(NET_REQUEST_VIRTUAL_INTERFACE_SET_PRIVATE_KEY, iface, &params,
		       sizeof(params));
	(void)memset(private_key, 0, sizeof(private_key));
	if (ret < 0) {
		return ret;
	}

	/* Grab a peer from the preallocated pool and wire up only the fields the
	 * transport receive path and the test getters touch. We deliberately do
	 * not bring the interface up nor run the Noise handshake: the tests inject
	 * transport-data messages straight into the receive path, decrypting with
	 * the synthetic session keys installed by wireguard_test_install_session().
	 */
	node = sys_slist_get(&peer_list);
	if (node == NULL) {
		return -ENOMEM;
	}

	peer = CONTAINER_OF(node, struct wg_peer, node);

	(void)memset(&peer->session, 0, sizeof(peer->session));

	peer->ctx = net_if_get_device(iface)->data;
	peer->iface = iface;
	peer->id = ++test_peer_id;
	peer->first_valid = false;
	peer->last_rx = 0;

	net_sin(net_sad(&peer->cfg_endpoint))->sin_family = NET_AF_INET;
	net_sin(net_sad(&peer->cfg_endpoint))->sin_port = net_htons(51820);
	net_sin(net_sad(&peer->cfg_endpoint))->sin_addr.s_addr = net_htonl(0xC0000201);
	memcpy(&peer->endpoint, &peer->cfg_endpoint, sizeof(peer->endpoint));

	sys_slist_prepend(&active_peers, node);

	if (iface_out != NULL) {
		*iface_out = iface;
	}

	return peer->id;
}

int wireguard_test_install_session(int peer_id,
				   const struct wireguard_test_session *session)
{
	struct wg_keypair *kp;
	struct wg_peer *peer;
	int ret;

	if (session == NULL) {
		return -EINVAL;
	}

	peer = peer_lookup_by_id(peer_id);
	if (peer == NULL) {
		return -ENOENT;
	}

	kp = &peer->session.keypair.current;

	keypair_destroy(kp);
	(void)memset(kp, 0, sizeof(*kp));

	ret = wg_psa_import_chacha_key(&kp->receiving_key_id, session->recv_key,
				       PSA_KEY_USAGE_DECRYPT);
	if (ret < 0) {
		return ret;
	}

	ret = wg_psa_import_chacha_key(&kp->sending_key_id, session->send_key,
				       PSA_KEY_USAGE_ENCRYPT);
	if (ret < 0) {
		keypair_destroy(kp);
		return ret;
	}

	kp->local_index = session->local_index;
	kp->remote_index = session->remote_index;
	kp->expires = sys_timepoint_calc(K_SECONDS(REJECT_AFTER_TIME));
	kp->rejected = sys_timepoint_calc(K_SECONDS(REJECT_AFTER_TIME * 3));
	kp->is_valid = true;
	kp->is_sending_valid = true;
	kp->is_receiving_valid = true;
	kp->is_initiator = true;

	peer->first_valid = false;

	return 0;
}

int wireguard_test_encrypt(const uint8_t key[32], uint64_t counter,
			   const uint8_t *plaintext, size_t plaintext_len,
			   uint8_t *ciphertext, size_t ciphertext_size,
			   size_t *ciphertext_len)
{
	size_t padded_len = ROUND_UP(plaintext_len, 16U);
	int ret;

	if (key == NULL || ciphertext == NULL || ciphertext_len == NULL) {
		return -EINVAL;
	}

	if (ciphertext_size < padded_len + WG_AUTHTAG_LEN) {
		return -ENOMEM;
	}

	(void)memset(ciphertext, 0, padded_len);
	if (plaintext_len > 0U && plaintext != NULL) {
		memcpy(ciphertext, plaintext, plaintext_len);
	}

	ret = wg_psa_aead_encrypt(ciphertext, ciphertext, padded_len, NULL, 0, counter, key);
	if (ret < 0) {
		return -EINVAL;
	}

	*ciphertext_len = padded_len + WG_AUTHTAG_LEN;

	return 0;
}

int wireguard_test_inject_transport(int peer_id,
				    const struct net_sockaddr *src,
				    uint64_t counter,
				    const uint8_t *ciphertext,
				    size_t ciphertext_len)
{
	NET_PKT_DATA_ACCESS_DEFINE(access, struct msg_transport_data);
	struct msg_transport_data *msg;
	struct msg_transport_data hdr;
	struct net_pkt *pkt = NULL;
	struct wg_peer *peer;
	size_t payload_len;
	int ret;

	if (src == NULL || ciphertext == NULL) {
		return -EINVAL;
	}

	if (src->sa_family != NET_AF_INET) {
		return -EAFNOSUPPORT;
	}

	peer = peer_lookup_by_id(peer_id);
	if (peer == NULL) {
		return -ENOENT;
	}

	payload_len = sizeof(struct msg_transport_data) + ciphertext_len;
	if (payload_len > CONFIG_WIREGUARD_BUF_LEN) {
		return -EMSGSIZE;
	}

	pkt = net_pkt_alloc_with_buffer(peer->iface, payload_len, NET_AF_UNSPEC, 0,
					PKT_ALLOC_WAIT_TIME);
	if (pkt == NULL) {
		return -ENOMEM;
	}

	hdr.type = MESSAGE_TRANSPORT_DATA;
	hdr.reserved[0] = hdr.reserved[1] = hdr.reserved[2] = 0;
	hdr.receiver = peer->session.keypair.current.local_index;
	sys_put_le64(counter, hdr.counter);

	ret = net_pkt_write(pkt, &hdr, sizeof(hdr));
	if (ret < 0) {
		goto out;
	}

	ret = net_pkt_write(pkt, ciphertext, ciphertext_len);
	if (ret < 0) {
		goto out;
	}

	net_pkt_cursor_init(pkt);

	msg = (struct msg_transport_data *)net_pkt_get_data(pkt, &access);
	if (msg == NULL) {
		ret = -EINVAL;
		goto out;
	}

	ret = wg_process_data_message(peer->ctx, peer, msg, pkt, 0,
				      (struct net_sockaddr *)src);

out:
	net_pkt_unref(pkt);

	return ret;
}

int wireguard_test_get_stats(int peer_id, struct net_stats_vpn *stats)
{
	struct wg_peer *peer;

	if (stats == NULL) {
		return -EINVAL;
	}

	peer = peer_lookup_by_id(peer_id);
	if (peer == NULL) {
		return -ENOENT;
	}

#if defined(CONFIG_NET_STATISTICS_VPN) && defined(CONFIG_NET_STATISTICS_USER_API)
	return net_mgmt(NET_REQUEST_STATS_GET_VPN, peer->iface, stats, sizeof(*stats));
#else
	return -ENOTSUP;
#endif
}

bool wireguard_test_peer_first_valid(int peer_id)
{
	struct wg_peer *peer = peer_lookup_by_id(peer_id);

	return peer != NULL ? peer->first_valid : false;
}

uint32_t wireguard_test_peer_last_rx(int peer_id)
{
	struct wg_peer *peer = peer_lookup_by_id(peer_id);

	return peer != NULL ? peer->last_rx : 0;
}

int wireguard_test_peer_endpoint(int peer_id, struct net_sockaddr_storage *endpoint)
{
	struct wg_peer *peer;

	if (endpoint == NULL) {
		return -EINVAL;
	}

	peer = peer_lookup_by_id(peer_id);
	if (peer == NULL) {
		return -ENOENT;
	}

	memcpy(endpoint, &peer->endpoint, sizeof(*endpoint));

	return 0;
}

int wireguard_test_peer_keypair_state(int peer_id,
				      struct wireguard_test_keypair_state *state)
{
	struct wg_peer *peer;

	if (state == NULL) {
		return -EINVAL;
	}

	peer = peer_lookup_by_id(peer_id);
	if (peer == NULL) {
		return -ENOENT;
	}

	state->prev_valid = peer->session.keypair.prev.is_valid;
	state->current_valid = peer->session.keypair.current.is_valid;
	state->next_valid = peer->session.keypair.next.is_valid;

	return 0;
}
