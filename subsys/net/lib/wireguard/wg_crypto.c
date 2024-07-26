/*
 * Copyright (c) 2021 Daniel Hope (www.floorsense.nz)
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *  list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this
 *  list of conditions and the following disclaimer in the documentation and/or
 *  other materials provided with the distribution.
 *
 * 3. Neither the name of "Floorsense Ltd", "Agile Workspace Ltd" nor the names of
 *  its contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Author: Daniel Hope <daniel.hope@smartalock.com>
 */

/* SPDX-License-Identifier: BSD-3-Clause */

/* Get the current time in tai64n format - 8 byte seconds, 4 byte nano sub-second -
 * see https://cr.yp.to/libtai/tai64.html for details
 * Output buffer passed is 12 bytes
 * The Wireguard implementation doesn't strictly need this to be a time,
 * but instead an increasing value.
 * The remote end of the Wireguard tunnel will use this value in handshake replay detection.
 */
void wg_tai64n_now(uint8_t *output)
{
	/* See https://cr.yp.to/libtai/tai64.html
	 * 64 bit seconds from 1970 = 8 bytes
	 * 32 bit nano seconds from current second
	 */
	uint64_t millis = k_ticks_to_ms_floor64(sys_clock_tick_get());

	/* Split into seconds offset + nanos */
	uint64_t seconds = 0x400000000000000aULL + (millis / 1000);
	uint32_t nanos = (millis % 1000) * 1000;

	sys_put_be64(seconds, output);
	sys_put_be32(nanos, output + 8U);
}

static void wg_mac(uint8_t *dst, const void *message, size_t len,
		   const uint8_t *key, size_t keylen)
{
	wireguard_blake2s(dst, WG_COOKIE_LEN, key, keylen, message, len);
}

static void wg_mac_key(uint8_t *key, const uint8_t *public_key,
		       const uint8_t *label, size_t label_len)
{
	wireguard_blake2s_ctx ctx;

	wireguard_blake2s_init(&ctx, WG_SESSION_KEY_LEN, NULL, 0);
	wireguard_blake2s_update(&ctx, label, label_len);
	wireguard_blake2s_update(&ctx, public_key, WG_PUBLIC_KEY_LEN);
	wireguard_blake2s_final(&ctx, key);
}

static void wg_mix_hash(uint8_t *hash, const uint8_t *src, size_t src_len)
{
	wireguard_blake2s_ctx ctx;

	wireguard_blake2s_init(&ctx, WG_HASH_LEN, NULL, 0);
	wireguard_blake2s_update(&ctx, hash, WG_HASH_LEN);
	wireguard_blake2s_update(&ctx, src, src_len);
	wireguard_blake2s_final(&ctx, hash);
}

static void wg_hmac(uint8_t *digest, const uint8_t *key, size_t key_len,
		    const uint8_t *data, size_t data_len)
{
	/* Adapted from appendix example in RFC2104 to use BLAKE2S instead
	 * of MD5 - https://tools.ietf.org/html/rfc2104
	 */
	uint8_t k_ipad[BLAKE2S_BLOCK_SIZE]; /* inner padding - key XORd with ipad */
	uint8_t k_opad[BLAKE2S_BLOCK_SIZE]; /* outer padding - key XORd with opad */
	wireguard_blake2s_ctx ctx;
	uint8_t tk[WG_HASH_LEN];

	/* if key is longer than BLAKE2S_BLOCK_SIZE bytes reset it to key=BLAKE2S(key) */
	if (key_len > BLAKE2S_BLOCK_SIZE) {
		wireguard_blake2s_ctx tctx;

		wireguard_blake2s_init(&tctx, WG_HASH_LEN, NULL, 0);
		wireguard_blake2s_update(&tctx, key, key_len);
		wireguard_blake2s_final(&tctx, tk);

		key = tk;
		key_len = WG_HASH_LEN;
	}

	/* The HMAC transform looks like:
	 * HASH(K XOR opad, HASH(K XOR ipad, text))
	 * where K is an n byte key
	 * ipad is the byte 0x36 repeated BLAKE2S_BLOCK_SIZE times
	 * opad is the byte 0x5c repeated BLAKE2S_BLOCK_SIZE times
	 * and text is the data being protected
	 */
	memset(k_ipad, 0, sizeof(k_ipad));
	memset(k_opad, 0, sizeof(k_opad));
	memcpy(k_ipad, key, key_len);
	memcpy(k_opad, key, key_len);

	/* XOR key with ipad and opad values */
	for (int i = 0; i < BLAKE2S_BLOCK_SIZE; i++) {
		k_ipad[i] ^= 0x36;
		k_opad[i] ^= 0x5c;
	}

	/* perform inner HASH */
	wireguard_blake2s_init(&ctx, WG_HASH_LEN, NULL, 0);  /* init context for 1st pass */
	wireguard_blake2s_update(&ctx, k_ipad, BLAKE2S_BLOCK_SIZE); /* start with inner pad */
	wireguard_blake2s_update(&ctx, data, data_len);      /* then text of datagram */
	wireguard_blake2s_final(&ctx, digest);               /* finish up 1st pass */

	/* perform outer HASH */
	wireguard_blake2s_init(&ctx, WG_HASH_LEN, NULL, 0);  /* init context for 2nd pass */
	wireguard_blake2s_update(&ctx, k_opad, BLAKE2S_BLOCK_SIZE); /* start with outer pad */
	wireguard_blake2s_update(&ctx, digest, WG_HASH_LEN); /* then results of 1st hash */
	wireguard_blake2s_final(&ctx, digest);               /* finish up 2nd pass */
}

static void wg_kdf1(uint8_t *tau1, const uint8_t *chaining_key,
		    const uint8_t *data, size_t data_len)
{
	uint8_t output[WG_HASH_LEN + 1];
	uint8_t tau0[WG_HASH_LEN];

	/* tau0 = Hmac(key, input) */
	wg_hmac(tau0, chaining_key, WG_HASH_LEN, data, data_len);

	/* tau1 := Hmac(tau0, 0x1) */
	output[0] = 1;
	wg_hmac(output, tau0, WG_HASH_LEN, output, 1);
	memcpy(tau1, output, WG_HASH_LEN);

	crypto_zero(tau0, sizeof(tau0));
	crypto_zero(output, sizeof(output));
}

static void wg_kdf2(uint8_t *tau1, uint8_t *tau2, const uint8_t *chaining_key,
		    const uint8_t *data, size_t data_len)
{
	uint8_t tau0[WG_HASH_LEN];
	uint8_t output[WG_HASH_LEN + 1];

	/* tau0 = Hmac(key, input) */
	wg_hmac(tau0, chaining_key, WG_HASH_LEN, data, data_len);

	/* tau1 := Hmac(tau0, 0x1) */
	output[0] = 1;
	wg_hmac(output, tau0, WG_HASH_LEN, output, 1);
	memcpy(tau1, output, WG_HASH_LEN);

	/* tau2 := Hmac(tau0,tau1 || 0x2) */
	output[WG_HASH_LEN] = 2;
	wg_hmac(output, tau0, WG_HASH_LEN, output, WG_HASH_LEN + 1);
	memcpy(tau2, output, WG_HASH_LEN);

	crypto_zero(tau0, sizeof(tau0));
	crypto_zero(output, sizeof(output));
}

static void wg_kdf3(uint8_t *tau1, uint8_t *tau2, uint8_t *tau3,
		    const uint8_t *chaining_key,
		    const uint8_t *data, size_t data_len)
{
	uint8_t tau0[WG_HASH_LEN];
	uint8_t output[WG_HASH_LEN + 1];

	/* tau0 = Hmac(key, input) */
	wg_hmac(tau0, chaining_key, WG_HASH_LEN, data, data_len);

	/* tau1 := Hmac(tau0, 0x1) */
	output[0] = 1;
	wg_hmac(output, tau0, WG_HASH_LEN, output, 1);
	memcpy(tau1, output, WG_HASH_LEN);

	/* tau2 := Hmac(tau0,tau1 || 0x2) */
	output[WG_HASH_LEN] = 2;
	wg_hmac(output, tau0, WG_HASH_LEN, output, WG_HASH_LEN + 1);
	memcpy(tau2, output, WG_HASH_LEN);

	/* tau3 := Hmac(tau0,tau1,tau2 || 0x3) */
	output[WG_HASH_LEN] = 3;
	wg_hmac(output, tau0, WG_HASH_LEN, output, WG_HASH_LEN + 1);
	memcpy(tau3, output, WG_HASH_LEN);

	crypto_zero(tau0, sizeof(tau0));
	crypto_zero(output, sizeof(output));
}

static bool wg_check_replay(struct wg_keypair *keypair, uint64_t seq)
{
	/* Implementation of packet replay window - as per RFC2401
	 * Adapted from code in Appendix C at https://tools.ietf.org/html/rfc2401
	 */
	size_t replay_window_size = sizeof(keypair->replay_bitmap) * CHAR_BIT; /* 32 bits */
	bool ret = false;
	uint32_t diff;

	/* WireGuard data packet counter starts from 0 but algorithm expects
	 * packet numbers to start from 1.
	 */
	seq++;

	if (seq == 0) {
		/* first == 0 or wrapped */
		return ret;
	}

	if (seq > keypair->replay_counter) {
		/* new larger sequence number */
		diff = seq - keypair->replay_counter;
		if (diff < replay_window_size) {
			/* In window */
			keypair->replay_bitmap <<= diff;

			/* set bit for this packet */
			keypair->replay_bitmap |= 1;
		} else {
			/* This packet has a "way larger" */
			keypair->replay_bitmap = 1;
		}
		keypair->replay_counter = seq;

		/* larger is good */
		ret = true;
	} else {
		diff = keypair->replay_counter - seq;
		if (diff < replay_window_size) {
			if (keypair->replay_bitmap & ((uint32_t)1 << diff)) {
				/* already seen */
				;
			} else {
				/* mark as seen */
				keypair->replay_bitmap |= ((uint32_t)1 << diff);

				/* out of order but good */
				ret = true;
			}
		} else {
			/* too old or wrapped */
			;
		}
	}

	return ret;
}

static void wg_clamp_private_key(uint8_t *key)
{
	key[0] &= 248;
	key[31] = (key[31] & 127) | 64;
}

static void wg_generate_private_key(uint8_t *key)
{
	(void)sys_csrand_get(key, WG_PRIVATE_KEY_LEN);
	wg_clamp_private_key(key);
}

static bool wg_generate_public_key(uint8_t *public_key, const uint8_t *private_key)
{
	bool ret = false;

	if (memcmp(private_key, zero_key, WG_PUBLIC_KEY_LEN) != 0) {
		ret = (wg_psa_x25519_public_key(public_key, private_key) == 0);
	}

	return ret;
}

static bool wg_check_mac1(struct wg_iface_context *ctx,
			  const uint8_t *data, size_t len,
			  const uint8_t *mac1)
{
	uint8_t calculated[WG_COOKIE_LEN];

	wg_mac(calculated, data, len, ctx->label_mac1_key, WG_SESSION_KEY_LEN);

	if (crypto_equal(calculated, mac1, WG_COOKIE_LEN)) {
		return true;
	}

	return false;
}

static void wg_generate_cookie_secret(struct wg_iface_context *ctx, uint32_t lifetime_in_ms)
{
	(void)sys_csrand_get(ctx->cookie_secret, sizeof(ctx->cookie_secret));

	ctx->cookie_secret_expires = sys_timepoint_calc(K_MSEC(lifetime_in_ms));
}

static void generate_peer_cookie(struct wg_iface_context *ctx,
				 uint8_t *cookie,
				 uint8_t *source_addr_port,
				 size_t source_length)
{
	wireguard_blake2s_ctx bl_ctx;

	if (sys_timepoint_expired(ctx->cookie_secret_expires)) {
		wg_generate_cookie_secret(ctx, COOKIE_SECRET_MAX_AGE_MSEC);
	}

	/* Mac(key, input) Keyed-Blake2s(key, input, 16), the keyed MAC variant
	 * of the BLAKE2s hash function, returning 16 bytes of output.
	 */
	wireguard_blake2s_init(&bl_ctx, WG_COOKIE_LEN, ctx->cookie_secret, WG_HASH_LEN);

	/* 5.4.7 Under Load: Cookie Reply Message */

	if (source_addr_port && (source_length > 0)) {
		wireguard_blake2s_update(&bl_ctx, source_addr_port, source_length);
	}

	wireguard_blake2s_final(&bl_ctx, cookie);
}

static bool wg_check_mac2(struct wg_iface_context *ctx,
			  const uint8_t *data, size_t len,
			  uint8_t *source_addr_port, size_t source_length,
			  const uint8_t *mac2)
{
	uint8_t calculated[WG_COOKIE_LEN];
	uint8_t cookie[WG_COOKIE_LEN];
	bool ret = false;

	generate_peer_cookie(ctx, cookie, source_addr_port, source_length);

	wg_mac(calculated, data, len, cookie, WG_COOKIE_LEN);

	if (crypto_equal(calculated, mac2, WG_COOKIE_LEN)) {
		ret = true;
	}

	return ret;
}

static void keypair_destroy(struct wg_keypair *keypair)
{
	crypto_zero(keypair, sizeof(struct wg_keypair));
	keypair->is_valid = false;
}

static void keypair_update(struct wg_peer *peer, struct wg_keypair *received_keypair)
{
	if (received_keypair == &peer->session.keypair.next) {
		peer->session.keypair.prev = peer->session.keypair.current;
		peer->session.keypair.current = peer->session.keypair.next;
		keypair_destroy(&peer->session.keypair.next);
	}
}

static void add_new_keypair(struct wg_peer *peer, struct wg_keypair *new_keypair)
{
	if (new_keypair->is_initiator) {
		if (peer->session.keypair.next.is_valid) {
			peer->session.keypair.prev = peer->session.keypair.next;
			keypair_destroy(&peer->session.keypair.next);
		} else  {
			peer->session.keypair.prev = peer->session.keypair.current;
		}

		memcpy(&peer->session.keypair.current, new_keypair,
		       sizeof(struct wg_keypair));
	} else {
		memcpy(&peer->session.keypair.next, new_keypair,
		       sizeof(struct wg_keypair));
		keypair_destroy(&peer->session.keypair.prev);
	}
}

static void wg_start_session(struct wg_peer *peer, bool is_initiator)
{
	struct wg_handshake *handshake = &peer->handshake;
	struct wg_keypair new_keypair;

	crypto_zero(&new_keypair, sizeof(struct wg_keypair));

	new_keypair.is_initiator = is_initiator;
	new_keypair.local_index = handshake->local_index;
	new_keypair.remote_index = handshake->remote_index;

	new_keypair.expires = sys_timepoint_calc(K_SECONDS(REJECT_AFTER_TIME));
	new_keypair.rejected = sys_timepoint_calc(K_SECONDS(REJECT_AFTER_TIME * 3));

	new_keypair.is_sending_valid = true;
	new_keypair.is_receiving_valid = true;

	/* 5.4.5 Transport Data Key Derivation
	 * (Tsendi = Trecvr, Trecvi = Tsendr) := Kdf2(Ci = Cr,E)
	 */
	if (new_keypair.is_initiator) {
		wg_kdf2(new_keypair.sending_key, new_keypair.receiving_key,
			handshake->chaining_key, NULL, 0);
	} else {
		wg_kdf2(new_keypair.receiving_key, new_keypair.sending_key,
			handshake->chaining_key, NULL, 0);
	}

	new_keypair.replay_bitmap = 0;
	new_keypair.replay_counter = 0;

	new_keypair.last_tx = 0;
	new_keypair.last_rx = 0; /* No packets received yet */

	new_keypair.is_valid = true;

	/* Eprivi = Epubi = Eprivr = Epubr = Ci = Cr := E */
	crypto_zero(handshake->ephemeral_private, WG_PUBLIC_KEY_LEN);
	crypto_zero(handshake->remote_ephemeral, WG_PUBLIC_KEY_LEN);
	crypto_zero(handshake->hash, WG_HASH_LEN);
	crypto_zero(handshake->chaining_key, WG_HASH_LEN);

	handshake->remote_index = 0;
	handshake->local_index = 0;
	handshake->is_valid = false;

	add_new_keypair(peer, &new_keypair);
}

/* We are the responder, other end is the initiator */
static struct wg_peer *wg_process_initiation_message(struct wg_iface_context *ctx,
						     struct msg_handshake_init *msg)
{
	struct wg_peer *ret_peer = NULL;
	struct wg_peer *peer;
	struct wg_handshake *handshake;
	uint8_t key[WG_SESSION_KEY_LEN];
	uint8_t chaining_key[WG_HASH_LEN];
	uint8_t hash[WG_HASH_LEN];
	uint8_t s[WG_PUBLIC_KEY_LEN];
	uint8_t e[WG_PUBLIC_KEY_LEN];
	uint8_t t[WG_TAI64N_LEN];
	uint8_t dh_calculation[WG_PUBLIC_KEY_LEN];
	uint32_t now;
	bool rate_limit;
	bool replay;

	/* Ci := Hash(Construction) (precalculated hash) */
	memcpy(chaining_key, ctx->wg_ctx->construction_hash, WG_HASH_LEN);

	/* Hi := Hash(Ci || Identifier */
	memcpy(hash, ctx->wg_ctx->identifier_hash, WG_HASH_LEN);

	/* Hi := Hash(Hi || Spubr) */
	wg_mix_hash(hash, ctx->public_key, WG_PUBLIC_KEY_LEN);

	/* Ci := Kdf1(Ci, Epubi) */
	wg_kdf1(chaining_key, chaining_key, msg->ephemeral, WG_PUBLIC_KEY_LEN);

	/* msg.ephemeral := Epubi */
	memcpy(e, msg->ephemeral, WG_PUBLIC_KEY_LEN);

	/* Hi := Hash(Hi || msg.ephemeral) */
	wg_mix_hash(hash, msg->ephemeral, WG_PUBLIC_KEY_LEN);

	/* Calculate DH(Eprivi,Spubr) */
	wireguard_x25519(dh_calculation, ctx->private_key, e);

	if (crypto_equal(dh_calculation, zero_key, WG_PUBLIC_KEY_LEN)) {
		NET_DBG("Bad X25519 (%d)", __LINE__);
		goto out;
	}

	/* (Ci,k) := Kdf2(Ci,DH(Eprivi,Spubr)) */
	wg_kdf2(chaining_key, key, chaining_key, dh_calculation, WG_PUBLIC_KEY_LEN);

	/* msg.static := AEAD(k, 0, Spubi, Hi) */
	if (!wireguard_aead_decrypt(s, msg->enc_static,
				    sizeof(msg->enc_static),
				    hash, WG_HASH_LEN, 0, key)) {
		NET_DBG("Failed to decrypt AEAD (%d)", __LINE__);
		goto out;
	}

	/* Hi := Hash(Hi || msg.static) */
	wg_mix_hash(hash, msg->enc_static, sizeof(msg->enc_static));

	peer = peer_lookup_by_pubkey(ctx, s);
	if (peer == NULL) {
		NET_DBG("No such peer");
		goto out;
	}

	handshake = &peer->handshake;

	/* (Ci,k) := Kdf2(Ci,DH(Sprivi,Spubr)) */
	wg_kdf2(chaining_key, key, chaining_key, peer->key.public_dh,
		WG_PUBLIC_KEY_LEN);

	/* msg.timestamp := AEAD(k, 0, Timestamp(), Hi) */
	if (!wireguard_aead_decrypt(t, msg->enc_timestamp,
				   sizeof(msg->enc_timestamp), hash,
				   WG_HASH_LEN, 0, key)) {
		NET_DBG("Failed to decrypt AEAD (%d)", __LINE__);
		goto out;
	}

	/* Hi := Hash(Hi || msg.timestamp) */
	wg_mix_hash(hash, msg->enc_timestamp, sizeof(msg->enc_timestamp));

	now = sys_clock_tick_get_32();

	/* Check that timestamp is increasing and we haven't had too many
	 * initiations (should only get one per peer every 5 seconds max?)
	 */
	/* tai64n is big endian so we can use memcmp to compare */
	replay = (memcmp(t, peer->greatest_timestamp, WG_TAI64N_LEN) <= 0);
	rate_limit = (peer->last_initiation_rx - now) < (1000 / MAX_INITIATIONS_PER_SECOND);
	if (replay || rate_limit) {
		NET_DBG("Too many initiations (replay %d, rate_limit %d)",
			replay, rate_limit);
		goto out;
	}

	/* Success! Copy everything to peer */
	peer->last_initiation_rx = now;

	if (memcmp(t, peer->greatest_timestamp, WG_TAI64N_LEN) > 0) {
		memcpy(peer->greatest_timestamp, t, WG_TAI64N_LEN);
		/* TODO: Need to notify if the higher layers want to
		 * persist latest timestamp/nonce somewhere.
		 */
	}

	memcpy(handshake->remote_ephemeral, e, WG_PUBLIC_KEY_LEN);
	memcpy(handshake->hash, hash, WG_HASH_LEN);
	memcpy(handshake->chaining_key, chaining_key, WG_HASH_LEN);

	handshake->remote_index = msg->sender;
	handshake->is_valid = true;
	handshake->is_initiator = false;
	ret_peer = peer;

out:
	crypto_zero(key, sizeof(key));
	crypto_zero(hash, sizeof(hash));
	crypto_zero(chaining_key, sizeof(chaining_key));
	crypto_zero(dh_calculation, sizeof(dh_calculation));

	return ret_peer;
}

static bool wg_process_handshake_response(struct wg_iface_context *ctx,
					  struct wg_peer *peer,
					  struct msg_handshake_response *src)
{
	struct wg_handshake *handshake = &peer->handshake;
	bool ret = false;
	uint8_t key[WG_SESSION_KEY_LEN];
	uint8_t hash[WG_HASH_LEN];
	uint8_t chaining_key[WG_HASH_LEN];
	uint8_t e[WG_PUBLIC_KEY_LEN];
	uint8_t ephemeral_private[WG_PUBLIC_KEY_LEN];
	uint8_t static_private[WG_PUBLIC_KEY_LEN];
	uint8_t preshared_key[WG_SESSION_KEY_LEN];
	uint8_t dh_calculation[WG_PUBLIC_KEY_LEN];
	uint8_t tau[WG_PUBLIC_KEY_LEN];

	if (!(handshake->is_valid && handshake->is_initiator)) {
		goto out;
	}

	memcpy(hash, handshake->hash, WG_HASH_LEN);
	memcpy(chaining_key, handshake->chaining_key, WG_HASH_LEN);
	memcpy(ephemeral_private, handshake->ephemeral_private, WG_PUBLIC_KEY_LEN);
	memcpy(preshared_key, peer->key.preshared, WG_SESSION_KEY_LEN);

	/* (Eprivr, Epubr) := DH-Generate()
	 * Not required
	 */

	/* Cr := Kdf1(Cr,Epubr) */
	wg_kdf1(chaining_key, chaining_key, src->ephemeral, WG_PUBLIC_KEY_LEN);

	/* msg.ephemeral := Epubr */
	memcpy(e, src->ephemeral, WG_PUBLIC_KEY_LEN);

	/* Hr := Hash(Hr || msg.ephemeral) */
	wg_mix_hash(hash, src->ephemeral, WG_PUBLIC_KEY_LEN);

	/* Cr := Kdf1(Cr, DH(Eprivr, Epubi))
	 * Calculate DH(Eprivr, Epubi)
	 */
	wireguard_x25519(dh_calculation, ephemeral_private, e);

	if (crypto_equal(dh_calculation, zero_key, WG_PUBLIC_KEY_LEN)) {
		NET_DBG("Bad X25519 (%d)", __LINE__);
		goto out;
	}

	wg_kdf1(chaining_key, chaining_key, dh_calculation, WG_PUBLIC_KEY_LEN);

	/* Cr := Kdf1(Cr, DH(Eprivr, Spubi))
	 * CalculateDH(Eprivr, Spubi)
	 */
	wireguard_x25519(dh_calculation, ctx->private_key, e);
	if (crypto_equal(dh_calculation, zero_key, WG_PUBLIC_KEY_LEN)) {
		NET_DBG("Bad X25519 (%d)", __LINE__);
		goto out;
	}

	wg_kdf1(chaining_key, chaining_key, dh_calculation, WG_PUBLIC_KEY_LEN);

	/* (Cr, t, k) := Kdf3(Cr, Q) */
	wg_kdf3(chaining_key, tau, key, chaining_key, peer->key.preshared,
		WG_SESSION_KEY_LEN);

	/* Hr := Hash(Hr | t) */
	wg_mix_hash(hash, tau, WG_HASH_LEN);

	/* msg.empty := AEAD(k, 0, E, Hr) */
	if (!wireguard_aead_decrypt(NULL, src->enc_empty, sizeof(src->enc_empty),
				    hash, WG_HASH_LEN, 0, key)) {
		NET_DBG("Failed to decrypt AEAD (%d)", __LINE__);
		goto out;
	}

	/* Hr := Hash(Hr | msg.empty)
	 * Not required as discarded
	 */

	memcpy(handshake->remote_ephemeral, e, WG_HASH_LEN);
	memcpy(handshake->hash, hash, WG_HASH_LEN);
	memcpy(handshake->chaining_key, chaining_key, WG_HASH_LEN);
	handshake->remote_index = src->sender;

	ret = true;

out:
	crypto_zero(key, sizeof(key));
	crypto_zero(hash, sizeof(hash));
	crypto_zero(chaining_key, sizeof(chaining_key));
	crypto_zero(ephemeral_private, sizeof(ephemeral_private));
	crypto_zero(static_private, sizeof(static_private));
	crypto_zero(preshared_key, sizeof(preshared_key));
	crypto_zero(tau, sizeof(tau));

	return ret;
}

static bool wg_process_cookie_message(struct wg_iface_context *ctx,
				      struct wg_peer *peer,
				      struct msg_cookie_reply *src)
{
	uint8_t cookie[WG_COOKIE_LEN];
	bool ret = false;

	if (!peer->handshake_mac1_valid) {
		NET_DBG("Handshake mac1 not valid");
		goto out;
	}

	ret = wireguard_xaead_decrypt(cookie,
				      src->enc_cookie,
				      sizeof(src->enc_cookie),
				      peer->handshake_mac1,
				      WG_COOKIE_LEN,
				      src->nonce,
				      peer->label_cookie_key);
	if (!ret) {
		NET_DBG("Failed to decrypt AEAD (%d)", __LINE__);
		goto out;
	}

	/* 5.4.7 Under Load: Cookie Reply Message
	 * Upon receiving this message, if it is valid, the only thing the
	 * recipient of this message should do is store the cookie along
	 * with the time at which it was received
	 */
	memcpy(peer->cookie, cookie, WG_COOKIE_LEN);

	peer->cookie_secret_expires = sys_timepoint_calc(K_MSEC(COOKIE_SECRET_MAX_AGE_MSEC));
	peer->handshake_mac1_valid = false;

out:
	return ret;
}

static bool wg_create_handshake_init(struct wg_iface_context *ctx,
				     struct wg_peer *peer,
				     struct msg_handshake_init *dst)
{
	struct wg_handshake *handshake = &peer->handshake;
	bool ret = false;
	uint8_t timestamp[WG_TAI64N_LEN];
	uint8_t key[WG_SESSION_KEY_LEN];
	uint8_t dh_calculation[WG_PUBLIC_KEY_LEN];

	memset(dst, 0, sizeof(struct msg_handshake_init));

	/* Ci := Hash(Construction) (precalculated hash) */
	memcpy(handshake->chaining_key, ctx->wg_ctx->construction_hash, WG_HASH_LEN);

	/* Hi := Hash(Ci || Identifier) */
	memcpy(handshake->hash, ctx->wg_ctx->identifier_hash, WG_HASH_LEN);

	/* Hi := Hash(Hi || Spubr) */
	wg_mix_hash(handshake->hash, peer->key.public_key, WG_PUBLIC_KEY_LEN);

	/* (Eprivi, Epubi) := DH-Generate() */
	wg_generate_private_key(handshake->ephemeral_private);
	if (!wg_generate_public_key(dst->ephemeral, handshake->ephemeral_private)) {
		NET_DBG("Cannot create public key");
		goto out;
	}

	/* Ci := Kdf1(Ci, Epubi) */
	wg_kdf1(handshake->chaining_key, handshake->chaining_key, dst->ephemeral,
		WG_PUBLIC_KEY_LEN);

	/* msg.ephemeral := Epubi
	 * Done above - public keys is calculated into dst->ephemeral
	 */

	/* Hi := Hash(Hi || msg.ephemeral) */
	wg_mix_hash(handshake->hash, dst->ephemeral, WG_PUBLIC_KEY_LEN);

	/* Calculate DH(Eprivi,Spubr) */
	wireguard_x25519(dh_calculation, handshake->ephemeral_private, peer->key.public_key);

	if (crypto_equal(dh_calculation, zero_key, WG_PUBLIC_KEY_LEN)) {
		NET_DBG("Bad X25519 (%d)", __LINE__);
		goto out;
	}

	/* (Ci,k) := Kdf2(Ci,DH(Eprivi,Spubr)) */
	wg_kdf2(handshake->chaining_key, key, handshake->chaining_key, dh_calculation,
		WG_PUBLIC_KEY_LEN);

	/* msg.static := AEAD(k,0,Spubi, Hi) */
	wireguard_aead_encrypt(dst->enc_static, ctx->public_key,
			       WG_PUBLIC_KEY_LEN, handshake->hash, WG_HASH_LEN, 0, key);

	/* Hi := Hash(Hi || msg.static) */
	wg_mix_hash(handshake->hash, dst->enc_static, sizeof(dst->enc_static));

	/* (Ci,k) := Kdf2(Ci,DH(Sprivi,Spubr))
	 * note DH(Sprivi,Spubr) is precomputed per peer
	 */
	wg_kdf2(handshake->chaining_key, key, handshake->chaining_key,
		peer->key.public_dh, WG_PUBLIC_KEY_LEN);

	/* msg.timestamp := AEAD(k, 0, Timestamp(), Hi) */
	wg_tai64n_now(timestamp);

	wireguard_aead_encrypt(dst->enc_timestamp, timestamp, WG_TAI64N_LEN,
			       handshake->hash, WG_HASH_LEN, 0, key);

	/* Hi := Hash(Hi || msg.timestamp) */
	wg_mix_hash(handshake->hash, dst->enc_timestamp, sizeof(dst->enc_timestamp));

	dst->type = MESSAGE_HANDSHAKE_INITIATION;
	dst->sender = generate_unique_index(ctx);

	handshake->is_valid = true;
	handshake->is_initiator = true;
	handshake->local_index = dst->sender;

	ret = true;

	/* 5.4.4 Cookie MACs
	 * msg.mac1 := Mac(Hash(Label-Mac1 || Spubm' ), msgA)
	 * The value Hash(Label-Mac1 || Spubm' ) above can be pre-computed
	 */
	wg_mac(dst->mac1, dst,
	       sizeof(struct msg_handshake_init) - (2 * WG_COOKIE_LEN),
	       peer->label_mac1_key, WG_SESSION_KEY_LEN);

	/* if Lm = E or Lm ≥ 120: */
	if ((peer->cookie_secret_expires.tick == 0ULL) ||
	    sys_timepoint_expired(peer->cookie_secret_expires)) {
		/* msg.mac2 := 0 */
		crypto_zero(dst->mac2, WG_COOKIE_LEN);
	} else {
		/* msg.mac2 := Mac(Lm, msgB) */
		wg_mac(dst->mac2, dst,
		       sizeof(struct msg_handshake_init) - WG_COOKIE_LEN,
		       peer->cookie, WG_COOKIE_LEN);
	}

out:
	crypto_zero(key, sizeof(key));
	crypto_zero(dh_calculation, sizeof(dh_calculation));

	return ret;
}

static bool wg_create_handshake_response(struct wg_iface_context *ctx,
					 struct wg_peer *peer,
					 struct msg_handshake_response *dst)
{
	struct wg_handshake *handshake = &peer->handshake;
	uint8_t dh_calculation[WG_PUBLIC_KEY_LEN];
	uint8_t key[WG_SESSION_KEY_LEN];
	uint8_t tau[WG_HASH_LEN];
	bool ret = false;

	memset(dst, 0, sizeof(struct msg_handshake_response));

	if (!(handshake->is_valid && !handshake->is_initiator)) {
		goto out;
	}

	/* (Eprivr, Epubr) := DH-Generate() */
	wg_generate_private_key(handshake->ephemeral_private);

	if (!wg_generate_public_key(dst->ephemeral, handshake->ephemeral_private)) {
		NET_DBG("Cannot generate public key");
		goto out;
	}

	/* Cr := Kdf1(Cr,Epubr) */
	wg_kdf1(handshake->chaining_key, handshake->chaining_key,
		dst->ephemeral, WG_PUBLIC_KEY_LEN);

	/* msg.ephemeral := Epubr
	 * Copied above when generated
	 */

	/* Hr := Hash(Hr || msg.ephemeral) */
	wg_mix_hash(handshake->hash, dst->ephemeral, WG_PUBLIC_KEY_LEN);

	/* Cr := Kdf1(Cr, DH(Eprivr, Epubi))
	 * Calculate DH(Eprivi,Spubr)
	 */
	wireguard_x25519(dh_calculation, handshake->ephemeral_private,
			 handshake->remote_ephemeral);

	if (crypto_equal(dh_calculation, zero_key, WG_PUBLIC_KEY_LEN)) {
		NET_DBG("Bad X25519 (%d)", __LINE__);
		goto out;
	}

	wg_kdf1(handshake->chaining_key, handshake->chaining_key,
		dh_calculation, WG_PUBLIC_KEY_LEN);

	/* Cr := Kdf1(Cr, DH(Eprivr, Spubi))
	 * Calculate DH(Eprivi,Spubr)
	 */
	wireguard_x25519(dh_calculation, handshake->ephemeral_private, peer->key.public_key);

	if (crypto_equal(dh_calculation, zero_key, WG_PUBLIC_KEY_LEN)) {
		NET_DBG("Bad X25519 (%d)", __LINE__);
		goto out;
	}

	wg_kdf1(handshake->chaining_key, handshake->chaining_key, dh_calculation,
		WG_PUBLIC_KEY_LEN);

	/* (Cr, t, k) := Kdf3(Cr, Q) */
	wg_kdf3(handshake->chaining_key, tau, key, handshake->chaining_key,
		peer->key.preshared, WG_SESSION_KEY_LEN);

	/* Hr := Hash(Hr | t) */
	wg_mix_hash(handshake->hash, tau, WG_HASH_LEN);

	/* msg.empty := AEAD(k, 0, E, Hr) */
	wireguard_aead_encrypt(dst->enc_empty, NULL, 0, handshake->hash,
			       WG_HASH_LEN, 0, key);

	/* Hr := Hash(Hr | msg.empty) */
	wg_mix_hash(handshake->hash, dst->enc_empty, sizeof(dst->enc_empty));

	dst->type = MESSAGE_HANDSHAKE_RESPONSE;
	dst->receiver = handshake->remote_index;
	dst->sender = generate_unique_index(ctx);

	/* Update handshake object too */
	handshake->local_index = dst->sender;

	ret = true;

	/* 5.4.4 Cookie MACs
	 * msg.mac1 := Mac(Hash(Label-Mac1 || Spubm' ), msgA)
	 * The value Hash(Label-Mac1 || Spubm' ) above can be pre-computed
	 */
	wg_mac(dst->mac1, dst,
	       (sizeof(struct msg_handshake_response) - (2 * WG_COOKIE_LEN)),
	       peer->label_mac1_key, WG_SESSION_KEY_LEN);

	/* if Lm = E or Lm ≥ 120: */
	if (sys_timepoint_expired(peer->cookie_secret_expires)) {
		/* msg.mac2 := 0 */
		crypto_zero(dst->mac2, WG_COOKIE_LEN);
	} else {
		/* msg.mac2 := Mac(Lm, msgB) */
		wg_mac(dst->mac2, dst,
		       (sizeof(struct msg_handshake_response) - WG_COOKIE_LEN),
		       peer->cookie, WG_COOKIE_LEN);
	}

out:
	crypto_zero(key, sizeof(key));
	crypto_zero(dh_calculation, sizeof(dh_calculation));
	crypto_zero(tau, sizeof(tau));

	return ret;
}

static void wg_create_cookie_reply(struct wg_iface_context *ctx,
				   struct msg_cookie_reply *dst,
				   const uint8_t *mac1,
				   uint32_t index,
				   uint8_t *source_addr_port,
				   size_t source_length)
{
	uint8_t cookie[WG_COOKIE_LEN];

	crypto_zero(dst, sizeof(struct msg_cookie_reply));

	dst->type = MESSAGE_COOKIE_REPLY;
	dst->receiver = index;

	(void)sys_csrand_get(dst->nonce, WG_COOKIE_NONCE_LEN);

	generate_peer_cookie(ctx, cookie, source_addr_port, source_length);

	wireguard_xaead_encrypt(dst->enc_cookie, cookie, WG_COOKIE_LEN,
				mac1, WG_COOKIE_LEN, dst->nonce,
				ctx->label_cookie_key);
}

static void wg_encrypt_packet(uint8_t *dst, const uint8_t *src, size_t src_len,
			      struct wg_keypair *keypair)
{
	wireguard_aead_encrypt(dst, src, src_len, NULL, 0,
			       keypair->sending_counter, keypair->sending_key);
	keypair->sending_counter++;
}

static bool wg_decrypt_packet(uint8_t *dst, const uint8_t *src, size_t src_len,
			      uint64_t counter, struct wg_keypair *keypair)
{
	return wireguard_aead_decrypt(dst, src, src_len, NULL, 0, counter,
				      keypair->receiving_key);
}

static bool wg_check_initiation_message(struct wg_iface_context *ctx,
					struct msg_handshake_init *msg,
					struct net_sockaddr *addr)
{
	bool ret = false;

	if (!wg_check_mac1(ctx, (uint8_t *)msg,
			   sizeof(struct msg_handshake_init) - (2 * WG_COOKIE_LEN),
			   msg->mac1)) {
		goto out;
	}

	if (!wg_is_under_load()) {
		/* If we aren't under load we only need mac1 to be correct */
		ret = true;
		goto out;
	}

	/* If we are under load then check mac2 */
	ret = wg_check_mac2(ctx, (uint8_t *)msg,
			    sizeof(struct msg_handshake_init) - WG_COOKIE_LEN,
			    (uint8_t *)&net_sin(addr)->sin_port,
			    addr->sa_family == NET_AF_INET ?
			    (2U + sizeof(struct net_in_addr)) :
			    (2U + sizeof(struct net_in6_addr)),
			    msg->mac2);
	if (!ret) {
		/* mac2 is invalid (cookie may have expired) or not present
		 * 5.3 Denial of Service Mitigation & Cookies
		 * If the responder receives a message with a valid msg.mac1
		 * yet with an invalid msg.mac2, and is under load, it may
		 * respond with a cookie reply message
		 */
		wg_send_handshake_cookie(ctx, msg->mac1, msg->sender, addr);
	}

out:
	return ret;
}

static bool wg_check_response_message(struct wg_iface_context *ctx,
				      struct msg_handshake_response *msg,
				      struct net_sockaddr *addr)
{
	bool ret;

	if (!wg_check_mac1(ctx, (uint8_t *)msg,
			   sizeof(struct msg_handshake_response) - (2 * WG_COOKIE_LEN),
			   msg->mac1)) {
		return false;
	}

	/* mac1 is valid! */
	if (!wg_is_under_load()) {
		/* If we aren't under load we only need mac1 to be correct */
		ret = true;
		goto out;
	}

	/* If we are under load then check mac2 */
	ret = wg_check_mac2(ctx, (uint8_t *)msg,
			    sizeof(struct msg_handshake_response) - (WG_COOKIE_LEN),
			    (uint8_t *)&net_sin(addr)->sin_port,
			    addr->sa_family == NET_AF_INET ?
			    (2U + sizeof(struct net_in_addr)) : (2U + sizeof(struct net_in6_addr)),
			    msg->mac2);
	if (!ret) {
		/* mac2 is invalid (cookie may have expired) or not present
		 * 5.3 Denial of Service Mitigation & Cookies
		 * If the responder receives a message with a valid msg.mac1 yet
		 * with an invalid msg.mac2, and is under load, it may respond
		 * with a cookie reply message
		 */
		wg_send_handshake_cookie(ctx, msg->mac1, msg->sender, addr);
	}

out:
	return ret;
}

static void wg_process_response_message(struct wg_iface_context *ctx,
					struct wg_peer *peer,
					struct msg_handshake_response *response,
					struct net_sockaddr *addr)
{
	bool ret;

	ret = wg_process_handshake_response(ctx, peer, response);
	if (!ret) {
		return;
	}

	update_peer_addr(peer, addr);

	wg_start_session(peer, true);

	(void)wg_send_keepalive(ctx, peer);
}

static int wg_process_data_message(struct wg_iface_context *ctx,
				   struct wg_peer *peer,
				   struct msg_transport_data *data_hdr,
				   struct net_pkt *pkt,
				   size_t ip_udp_hdr_len,
				   struct net_sockaddr *addr)
{
	struct net_pkt *pkt_decrypted = NULL;
	uint32_t idx = data_hdr->receiver;
	struct net_buf *buf = NULL;
	struct wg_keypair *keypair;
	enum net_verdict verdict;
	uint16_t pkt_len;
	uint8_t vtc_vhl;
	uint64_t nonce;
	size_t data_len;
	size_t copied;
	bool addr_found;
	bool status;
	int ret;

	keypair = get_peer_keypair_for_index(peer, idx);
	if (keypair == NULL) {
		return -ENOENT;
	}

	if (!(keypair->is_receiving_valid &&
	      !sys_timepoint_expired(keypair->expires) &&
	      keypair->sending_counter < REJECT_AFTER_MESSAGES)) {
		/* After Reject-After-Messages transport data messages or after
		 * the current secure session is Reject-After-Time seconds old,
		 * which ever comes first, WireGuard will refuse to send or
		 * receive any more transport data messages using the current
		 * secure session, until a new secure session is created
		 * through the 1-RTT handshake.
		 */
		keypair_destroy(keypair);

		return -EKEYEXPIRED;
	}

	nonce = sys_get_le64(data_hdr->counter);

	/* We need to linearize the packet before we can decrypt it.
	 * TODO: make crypto support data in multiple net_buf's so we
	 * don't need to linearize the packet.
	 */
	data_len = net_pkt_get_len(pkt) - sizeof(struct msg_transport_data) - ip_udp_hdr_len;

	/* If the data_len is 16 bytes (padded length), then it is a keepalive message */
	if (data_len == 16U) {
		/* keep-alive message */
		NET_DBG("Keepalive message received from %s",
			net_sprint_addr(addr->sa_family,
					(const void *)&net_sin(addr)->sin_addr));
		return 0;
	}

	/* We don't know the unpadded size until we have decrypted the packet
	 * and validated/inspected the IP header
	 */
	buf = net_buf_alloc(&msg_pool, BUF_ALLOC_TIMEOUT);
	if (buf == NULL) {
		NET_DBG("Failed to allocate %s buffer", "decrypt");
		return -ENOMEM;
	}

	copied = net_buf_linearize(buf->data, data_len, pkt->buffer,
				   ip_udp_hdr_len + sizeof(struct msg_transport_data),
				   data_len);
	if (copied != data_len) {
		NET_DBG("Failed to linearize data (%zu != %zu)", copied, data_len);
		ret = -EMSGSIZE;
		goto out;
	}

	data_len = copied;

	NET_DBG("Transport data %zu bytes", data_len);

	status = wg_decrypt_packet(buf->data, buf->data, data_len, nonce, keypair);
	if (!status) {
		ret = -ENOMSG;
		goto out;
	}

	/* Since the packet has authenticated correctly, the source IP of
	 * the outer UDP/IP packet is used to update the endpoint for peer.
	 */

	update_peer_addr(peer, addr);

	keypair->last_rx = peer->last_rx = sys_clock_tick_get_32();

	/* Might need to shuffle next key --> current keypair */
	keypair_update(peer, keypair);

	/* Check to see if we should rekey */
	if (keypair->sending_counter >= REKEY_AFTER_MESSAGES) {
		peer->send_handshake = true;
	} else if (keypair->is_initiator &&
		   (k_ticks_to_sec_floor32(
			   sys_timepoint_timeout(keypair->expires).ticks) > REKEY_AFTER_TIME)) {
		peer->send_handshake = true;
	}

	/* Check for packet replay / dupes */
	if (!wg_check_replay(keypair, nonce)) {
		ret = -EINVAL;
		goto out;
	}

	/* We then need to copy the decrypted data into a net_pkt so that it can
	 * be processed by the network stack.
	 */
	pkt_decrypted = net_pkt_alloc_from_slab(&decrypted_pkts, PKT_ALLOC_WAIT_TIME);
	if (pkt_decrypted == NULL) {
		NET_DBG("Failed to allocate packet");
		ret = -ENOMEM;
		goto out;
	}

	net_pkt_append_buffer(pkt_decrypted, buf);
	pkt_decrypted->buffer->len = data_len;
	net_pkt_cursor_init(pkt_decrypted);
	net_pkt_set_overwrite(pkt_decrypted, true);

	buf = NULL;
	addr_found = false;

	vtc_vhl = NET_IPV6_HDR(pkt_decrypted)->vtc & 0xf0;

	if (IS_ENABLED(CONFIG_NET_IPV6) && vtc_vhl == 0x60) {
		net_pkt_set_ip_hdr_len(pkt_decrypted, sizeof(struct net_ipv6_hdr));
		net_pkt_set_ipv6_ext_len(pkt_decrypted, 0);
		pkt_len = net_ntohs(NET_IPV6_HDR(pkt_decrypted)->len) +
			sizeof(struct net_ipv6_hdr);

		ARRAY_FOR_EACH(peer->allowed_ip, i) {
			if (!(peer->allowed_ip[i].is_valid &&
			      peer->allowed_ip[i].addr.family == NET_AF_INET6)) {
				continue;
			}

			if (net_ipv6_is_prefix(
				    (const uint8_t *)&NET_IPV6_HDR(pkt_decrypted)->src,
				    (const uint8_t *)&peer->allowed_ip[i].addr.in6_addr,
				    peer->allowed_ip[i].mask_len)) {
				addr_found = true;
				break;
			}
		}

		if (!addr_found) {
			NET_DBG("Address %s not found in allowed list",
				net_sprint_ipv6_addr((const struct in6_addr *)
						     &NET_IPV6_HDR(pkt_decrypted)->src));
		}

	} else if (IS_ENABLED(CONFIG_NET_IPV4) && vtc_vhl == 0x40) {
		struct net_in_addr src;
		uint32_t subnet;

		net_pkt_set_ip_hdr_len(pkt_decrypted, sizeof(struct net_ipv4_hdr));
		net_pkt_set_ipv4_opts_len(pkt_decrypted, 0);
		pkt_len = net_ntohs(NET_IPV4_HDR(pkt_decrypted)->len);

		ARRAY_FOR_EACH(peer->allowed_ip, i) {
			if (!(peer->allowed_ip[i].is_valid &&
			      peer->allowed_ip[i].addr.family == NET_AF_INET)) {
				continue;
			}

			src.s_addr = sys_get_be32(
				(const uint8_t *)&NET_IPV4_HDR(pkt_decrypted)->src);
			subnet = UINT32_MAX << (32 - peer->allowed_ip[i].mask_len);

			if ((src.s_addr & subnet) ==
			    (net_ntohl(peer->allowed_ip[i].addr.in_addr.s_addr) & subnet)) {
				addr_found = true;
				break;
			}
		}

		if (!addr_found) {
			NET_DBG("Address %s not found in allowed list",
				net_sprint_ipv4_addr((const struct in_addr *)
						     &NET_IPV4_HDR(pkt_decrypted)->src));
		}

	} else {
		NET_DBG("Unknown IP version %d", vtc_vhl >> 4);
		ret = -EINVAL;
		goto out;
	}

	if (!addr_found) {
		ret = -ENOENT;
		goto out;
	}

	if (data_len < pkt_len) {
		NET_DBG("Packet length mismatch %d < %d", data_len, pkt_len);
		ret = -EINVAL;
		goto out;
	}

	if (data_len != pkt_len) {
		/* We need to adjust the length of the packet to match the
		 * decrypted data
		 */
		pkt_decrypted->buffer->len = pkt_len;

		NET_DBG("Packet length adjusted %d -> %d", data_len, pkt_len);
	}

	net_pkt_set_iface(pkt_decrypted, ctx->iface);

	/* The packet is now ready to be passed to the network stack */
	verdict = net_if_l2(ctx->iface)->recv(ctx->iface, pkt_decrypted);
	if (verdict == NET_DROP) {
		ret = -ENETRESET;
		goto out;
	} else {
		/* The packet has been consumed by the core stack,
		 * don't free it.
		 */
		pkt_decrypted = NULL;
	}

	ret = 0;
out:
	if (pkt_decrypted != NULL) {
		net_pkt_unref(pkt_decrypted);
	}

	if (buf != NULL) {
		net_buf_unref(buf);
	}

	return ret;
}
