/*
 *  SPDX-FileCopyrightText: 2026 Basalte bv
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

/** @file srtp.c
 *
 * @brief SRTP packet protection as defined in RFC 3711 and RFC 7714.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(srtp, CONFIG_RTP_LOG_LEVEL);

#include <zephyr/net/net_log.h>
#include <zephyr/net/rtp.h>
#include <zephyr/net/srtp.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "srtp_crypto.h"

#define LOCK_TIMEOUT K_MSEC(100)

/* Each master key may protect at most 2^48 SRTP packets (RFC 3711 3.3.1) */
#define SRTP_MAX_PACKETS BIT64(48)

/* The key derivation rate must be 0 or 2^i for i in 0..24 (RFC 3711 4.3.1) */
#define SRTP_KDR_MAX BIT(24)

/* Half the sequence number space, used for the index estimation (RFC 3711
 * appendix A).
 */
#define SRTP_SEQ_HALF_RANGE 0x8000

BUILD_ASSERT(CONFIG_SRTP_REPLAY_WINDOW_SIZE % SRTP_REPLAY_WINDOW_WORD_BITS == 0,
	     "CONFIG_SRTP_REPLAY_WINDOW_SIZE must be a multiple of the bitmap word size");

static size_t srtp_auth_tag_len(enum srtp_auth auth)
{
	switch (auth) {
	case SRTP_AUTH_HMAC_SHA1_80:
		return 10;
	case SRTP_AUTH_HMAC_SHA1_32:
		return 4;
	case SRTP_AUTH_NULL:
	default:
		return 0;
	}
}

static bool srtp_cipher_is_aead(enum srtp_cipher cipher)
{
	return cipher == SRTP_CIPHER_AES_128_GCM || cipher == SRTP_CIPHER_AES_256_GCM;
}

static size_t srtp_cipher_key_len(enum srtp_cipher cipher)
{
	return cipher == SRTP_CIPHER_AES_256_GCM ? SRTP_AES_256_KEY_LEN : SRTP_AES_128_KEY_LEN;
}

/* Serialized RTP header length, directly from the wire representation */
static int srtp_raw_hdr_len(const uint8_t *buf, size_t len, size_t *hdr_len)
{
	size_t total;

	if (len < RTP_MIN_HEADER_LEN) {
		return -EBADMSG;
	}

	total = RTP_MIN_HEADER_LEN + FIELD_GET(RTP_HDR_CC_MASK, buf[0]) * sizeof(uint32_t);

	if (FIELD_GET(RTP_HDR_X_MASK, buf[0]) == 1) {
		if (len < total + 2 * sizeof(uint16_t)) {
			return -EBADMSG;
		}

		total += 2 * sizeof(uint16_t) +
			 sys_get_be16(&buf[total + sizeof(uint16_t)]) * sizeof(uint32_t);
	}

	if (total > len) {
		return -EBADMSG;
	}

	*hdr_len = total;

	return 0;
}

/*
 * Estimate the 48-bit packet index of a sequence number (RFC 3711 3.3.1 and
 * Appendix A). Fails when the index precedes the start of the stream or when
 * the rollover counter would exceed 32 bits.
 */
static int srtp_index_estimate(const struct srtp_stream *stream, uint16_t seq, uint32_t *v,
			       uint64_t *index)
{
	int64_t guess = stream->roc;

	if (stream->seq_initialized) {
		if (stream->s_l < SRTP_SEQ_HALF_RANGE) {
			if ((int32_t)seq - (int32_t)stream->s_l > SRTP_SEQ_HALF_RANGE) {
				guess--;
			}
		} else {
			if ((int32_t)stream->s_l - SRTP_SEQ_HALF_RANGE > (int32_t)seq) {
				guess++;
			}
		}
	}

	if (guess < 0) {
		return -EALREADY;
	}

	if (guess > UINT32_MAX) {
		return -EKEYEXPIRED;
	}

	*v = (uint32_t)guess;
	*index = ((uint64_t)*v << 16) | seq;

	return 0;
}

static int srtp_replay_check(const struct srtp_stream *stream, uint64_t index)
{
	uint64_t delta;

	if (index > stream->index_max) {
		return 0;
	}

	delta = stream->index_max - index;

	if (delta >= CONFIG_SRTP_REPLAY_WINDOW_SIZE) {
		return -EALREADY;
	}

	if ((stream->replay_window[delta / SRTP_REPLAY_WINDOW_WORD_BITS] &
	     BIT64(delta % SRTP_REPLAY_WINDOW_WORD_BITS)) != 0) {
		return -EALREADY;
	}

	return 0;
}

static void srtp_window_shift(uint64_t *window, uint64_t shift)
{
	const size_t word_shift = shift / SRTP_REPLAY_WINDOW_WORD_BITS;
	const size_t bit_shift = shift % SRTP_REPLAY_WINDOW_WORD_BITS;

	if (shift >= CONFIG_SRTP_REPLAY_WINDOW_SIZE) {
		memset(window, 0, SRTP_REPLAY_WINDOW_WORDS * sizeof(uint64_t));
		return;
	}

	for (size_t i = SRTP_REPLAY_WINDOW_WORDS; i-- > 0;) {
		uint64_t val = 0;

		if (i >= word_shift) {
			val = window[i - word_shift] << bit_shift;
			if (bit_shift > 0 && i > word_shift) {
				val |= window[i - word_shift - 1] >>
				       (SRTP_REPLAY_WINDOW_WORD_BITS - bit_shift);
			}
		}

		window[i] = val;
	}
}

static void srtp_replay_advance(struct srtp_stream *stream, uint64_t index)
{
	if (index > stream->index_max) {
		srtp_window_shift(stream->replay_window, index - stream->index_max);
		stream->replay_window[0] |= BIT64(0);
		stream->index_max = index;
	} else {
		uint64_t delta = stream->index_max - index;

		stream->replay_window[delta / SRTP_REPLAY_WINDOW_WORD_BITS] |=
			BIT64(delta % SRTP_REPLAY_WINDOW_WORD_BITS);
	}
}

/* Initial counter block for AES-CM (RFC 3711 4.1.1) */
static void srtp_cm_iv(const uint8_t *k_s, uint32_t ssrc, uint64_t index, uint8_t iv[16])
{
	uint8_t x[16] = {0};

	sys_put_be32(ssrc, &x[4]);
	sys_put_be48(index, &x[8]);
	mem_xor_n(x, x, k_s, SRTP_MASTER_SALT_LEN);

	memcpy(iv, x, sizeof(x));
}

/* Initialization vector for AES-GCM (RFC 7714 8.1) */
static void srtp_gcm_iv(const uint8_t *k_s, uint32_t ssrc, uint32_t roc, uint16_t seq,
			uint8_t iv[SRTP_AES_GCM_IV_LEN])
{
	uint8_t x[SRTP_AES_GCM_IV_LEN] = {0};

	sys_put_be32(ssrc, &x[2]);
	sys_put_be32(roc, &x[6]);
	sys_put_be16(seq, &x[10]);

	mem_xor_n(iv, k_s, x, SRTP_AES_GCM_IV_LEN);
}

static bool srtp_needs_keys(const struct srtp_stream *stream)
{
	return stream->cipher != SRTP_CIPHER_NULL || stream->auth != SRTP_AUTH_NULL;
}

/* A set of session keys derived for one key segment. Kept separate from the
 * stream so keys for an unauthenticated packet can be derived without
 * mutating the stream state.
 */
struct srtp_keyset {
	psa_key_id_t k_e;
	psa_key_id_t k_a;
	uint8_t k_s[SRTP_MASTER_SALT_LEN];
};

static void srtp_keyset_destroy(struct srtp_keyset *keys)
{
	srtp_crypto_key_destroy(&keys->k_e);
	srtp_crypto_key_destroy(&keys->k_a);
	srtp_crypto_zeroize(keys->k_s, sizeof(keys->k_s));
}

static int srtp_keyset_derive(const struct srtp_stream *stream, uint64_t segment,
			      struct srtp_keyset *keys)
{
	uint8_t key[MAX(SRTP_AES_256_KEY_LEN, SRTP_HMAC_SHA1_KEY_LEN)];
	int ret = 0;

	memset(keys, 0, sizeof(*keys));

	if (stream->cipher != SRTP_CIPHER_NULL) {
		size_t key_len = srtp_cipher_key_len(stream->cipher);

		ret = srtp_crypto_kdf(stream->master_key, stream->master_salt,
				      SRTP_KDF_LABEL_ENCRYPTION, segment, key, key_len);
		if (ret == 0) {
			if (srtp_cipher_is_aead(stream->cipher)) {
				ret = srtp_crypto_gcm_key_import(key, key_len, &keys->k_e);
			} else {
				ret = srtp_crypto_cm_key_import(key, &keys->k_e);
			}
		}
		if (ret < 0) {
			goto out;
		}

		ret = srtp_crypto_kdf(stream->master_key, stream->master_salt, SRTP_KDF_LABEL_SALT,
				      segment, keys->k_s, stream->master_salt_len);
		if (ret < 0) {
			goto out;
		}
	}

	if (stream->auth != SRTP_AUTH_NULL) {
		ret = srtp_crypto_kdf(stream->master_key, stream->master_salt, SRTP_KDF_LABEL_AUTH,
				      segment, key, SRTP_HMAC_SHA1_KEY_LEN);
		if (ret == 0) {
			ret = srtp_crypto_auth_key_import(key, srtp_auth_tag_len(stream->auth),
							  &keys->k_a);
		}
	}

out:
	if (ret < 0) {
		srtp_keyset_destroy(keys);
	}
	srtp_crypto_zeroize(key, sizeof(key));
	return ret;
}

static void srtp_keyset_commit(struct srtp_stream *stream, struct srtp_keyset *keys,
			       uint64_t segment)
{
	srtp_crypto_key_destroy(&stream->k_e);
	srtp_crypto_key_destroy(&stream->k_a);

	stream->k_e = keys->k_e;
	stream->k_a = keys->k_a;
	memcpy(stream->k_s, keys->k_s, sizeof(stream->k_s));
	stream->key_segment = segment;
}

static int srtp_derive_session_keys(struct srtp_stream *stream, uint64_t segment)
{
	struct srtp_keyset keys;
	int ret;

	ret = srtp_keyset_derive(stream, segment, &keys);
	if (ret == 0) {
		srtp_keyset_commit(stream, &keys, segment);
	}

	return ret;
}

static int srtp_refresh_session_keys(struct srtp_stream *stream, uint64_t index)
{
	uint64_t segment;

	if (stream->kdr == 0 || !srtp_needs_keys(stream)) {
		return 0;
	}

	segment = index / stream->kdr;

	if (segment == stream->key_segment) {
		return 0;
	}

	return srtp_derive_session_keys(stream, segment);
}

int srtp_policy_validate(const struct srtp_policy *policy)
{
	if (policy == NULL) {
		return -EINVAL;
	}

	switch (policy->cipher) {
	case SRTP_CIPHER_NULL:
		break;
	case SRTP_CIPHER_AES_128_CM:
		if (policy->auth == SRTP_AUTH_NULL && !policy->allow_null_auth) {
			NET_DBG("NULL auth requires explicit opt-in");
			return -EINVAL;
		}
		break;
	case SRTP_CIPHER_AES_128_GCM:
	case SRTP_CIPHER_AES_256_GCM:
		if (policy->auth != SRTP_AUTH_NULL) {
			NET_DBG("AEAD ciphers provide authentication themselves");
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	switch (policy->auth) {
	case SRTP_AUTH_NULL:
		if (policy->cipher == SRTP_CIPHER_NULL && !policy->allow_null_auth) {
			NET_DBG("NULL auth requires explicit opt-in");
			return -EINVAL;
		}
		break;
	case SRTP_AUTH_HMAC_SHA1_80:
	case SRTP_AUTH_HMAC_SHA1_32:
		break;
	default:
		return -EINVAL;
	}

	if (policy->cipher != SRTP_CIPHER_NULL || policy->auth != SRTP_AUTH_NULL ||
	    policy->master_key != NULL || policy->master_salt != NULL) {
		size_t salt_len = srtp_cipher_is_aead(policy->cipher) ? SRTP_AES_GCM_SALT_LEN
								      : SRTP_MASTER_SALT_LEN;

		if (policy->master_key == NULL ||
		    policy->master_key_len != srtp_cipher_key_len(policy->cipher)) {
			NET_DBG("Invalid master key");
			return -EINVAL;
		}

		if (policy->master_salt == NULL || policy->master_salt_len != salt_len) {
			NET_DBG("Invalid master salt");
			return -EINVAL;
		}
	}

	if ((policy->mki == NULL) != (policy->mki_len == 0) || policy->mki_len > SRTP_MKI_MAX_LEN) {
		NET_DBG("Invalid MKI");
		return -EINVAL;
	}

	if (policy->kdr != 0 &&
	    (policy->kdr > SRTP_KDR_MAX || (policy->kdr & (policy->kdr - 1)) != 0)) {
		NET_DBG("Key derivation rate must be 0 or a power of two up to 2^24");
		return -EINVAL;
	}

	return 0;
}

/* Common protect/unprotect entry: header walk, SSRC match, and trailer length */
static int srtp_packet_check(const struct srtp_stream *stream, const uint8_t *buf, size_t pkt_len,
			     size_t *hdr_len, size_t *tag_len, uint16_t *seq)
{
	int ret;

	ret = srtp_raw_hdr_len(buf, pkt_len, hdr_len);
	if (ret < 0) {
		return ret;
	}

	*seq = sys_get_be16(&buf[2]);

	if (sys_get_be32(&buf[8]) != stream->ssrc) {
		NET_DBG("Packet SSRC does not match stream");
		return -EINVAL;
	}

	*tag_len = srtp_cipher_is_aead(stream->cipher) ? SRTP_AES_GCM_TAG_LEN
						       : srtp_auth_tag_len(stream->auth);

	return 0;
}

int srtp_stream_init(struct srtp_stream *stream, const struct srtp_policy *policy, uint32_t ssrc)
{
	int ret;

	if (stream == NULL || policy == NULL) {
		return -EINVAL;
	}

	ret = srtp_policy_validate(policy);
	if (ret < 0) {
		return ret;
	}

	memset(stream, 0, sizeof(*stream));
	stream->ssrc = ssrc;
	stream->cipher = policy->cipher;
	stream->auth = policy->auth;
	stream->kdr = policy->kdr;

	if (policy->mki_len > 0) {
		memcpy(stream->mki, policy->mki, policy->mki_len);
		stream->mki_len = policy->mki_len;
	}

	k_mutex_init(&stream->lock);

	if (!srtp_needs_keys(stream)) {
		return 0;
	}

	ret = srtp_crypto_master_key_import(policy->master_key, policy->master_key_len,
					    &stream->master_key);
	if (ret < 0) {
		return ret;
	}

	/* Shorter (AES-GCM) master salts are zero padded for the KDF (RFC 7714 5.1) */
	memcpy(stream->master_salt, policy->master_salt, policy->master_salt_len);
	stream->master_salt_len = policy->master_salt_len;

	ret = srtp_derive_session_keys(stream, 0);
	if (ret < 0) {
		(void)srtp_stream_deinit(stream);
		return ret;
	}

	return 0;
}

int srtp_stream_deinit(struct srtp_stream *stream)
{
	if (stream == NULL) {
		return -EINVAL;
	}

	srtp_crypto_key_destroy(&stream->k_e);
	srtp_crypto_key_destroy(&stream->k_a);
	srtp_crypto_key_destroy(&stream->master_key);

	srtp_crypto_zeroize(stream->master_salt, sizeof(stream->master_salt));
	srtp_crypto_zeroize(stream->k_s, sizeof(stream->k_s));

	return 0;
}

int srtp_protect(struct srtp_stream *stream, uint8_t *buf, size_t pkt_len, size_t buf_size,
		 size_t *out_len)
{
	size_t hdr_len;
	size_t tag_len;
	size_t len = pkt_len;
	size_t auth_len;
	uint64_t index;
	uint32_t v;
	uint16_t seq;
	int ret;

	if (stream == NULL || buf == NULL || out_len == NULL || buf_size < pkt_len) {
		return -EINVAL;
	}

	ret = srtp_packet_check(stream, buf, pkt_len, &hdr_len, &tag_len, &seq);
	if (ret < 0) {
		return ret;
	}

	if (buf_size - pkt_len < tag_len + stream->mki_len) {
		NET_DBG("No room for the SRTP trailer");
		return -ENOMEM;
	}

	ret = k_mutex_lock(&stream->lock, LOCK_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

	if (stream->tx_count >= SRTP_MAX_PACKETS) {
		ret = -EKEYEXPIRED;
		goto unlock;
	}

	ret = srtp_index_estimate(stream, seq, &v, &index);
	if (ret < 0) {
		/* Transmit sequence numbers never move backwards, so the
		 * estimation can only fail with an exhausted rollover counter.
		 */
		ret = -EKEYEXPIRED;
		goto unlock;
	}

	if (stream->seq_initialized && index <= stream->index_max) {
		NET_DBG("Sequence numbers must increase strictly on transmit");
		ret = -EINVAL;
		goto unlock;
	}

	ret = srtp_refresh_session_keys(stream, index);
	if (ret < 0) {
		goto unlock;
	}

	switch (stream->cipher) {
	case SRTP_CIPHER_AES_128_CM: {
		uint8_t iv[16];

		srtp_cm_iv(stream->k_s, stream->ssrc, index, iv);
		ret = srtp_crypto_cm_crypt(stream->k_e, iv, &buf[hdr_len], len - hdr_len);
		break;
	}
	case SRTP_CIPHER_AES_128_GCM:
	case SRTP_CIPHER_AES_256_GCM: {
		uint8_t iv[SRTP_AES_GCM_IV_LEN];

		srtp_gcm_iv(stream->k_s, stream->ssrc, v, seq, iv);
		ret = srtp_crypto_gcm_encrypt(stream->k_e, iv, buf, hdr_len, &buf[hdr_len],
					      len - hdr_len);
		len += SRTP_AES_GCM_TAG_LEN;
		break;
	}
	case SRTP_CIPHER_NULL:
	default:
		ret = 0;
		break;
	}

	if (ret < 0) {
		goto unlock;
	}

	auth_len = len;

	if (stream->mki_len > 0) {
		memcpy(&buf[len], stream->mki, stream->mki_len);
		len += stream->mki_len;
	}

	if (stream->auth != SRTP_AUTH_NULL) {
		/* The MKI is not part of the authenticated portion (RFC 3711 4.2) */
		ret = srtp_crypto_auth_compute(stream->k_a, srtp_auth_tag_len(stream->auth), buf,
					       auth_len, v, &buf[len]);
		if (ret < 0) {
			goto unlock;
		}

		len += srtp_auth_tag_len(stream->auth);
	}

	stream->roc = v;
	stream->s_l = seq;
	stream->seq_initialized = true;
	stream->index_max = index;
	stream->tx_count++;

	*out_len = len;
	ret = 0;

unlock:
	(void)k_mutex_unlock(&stream->lock);
	return ret;
}

int srtp_unprotect(struct srtp_stream *stream, uint8_t *buf, size_t pkt_len, size_t *out_len)
{
	size_t hdr_len;
	size_t tag_len;
	struct srtp_keyset cand;
	bool cand_keys = false;
	const uint8_t *k_s;
	psa_key_id_t k_e;
	psa_key_id_t k_a;
	uint64_t index;
	uint32_t v;
	uint16_t seq;
	int ret;

	if (stream == NULL || buf == NULL || out_len == NULL) {
		return -EINVAL;
	}

	ret = srtp_packet_check(stream, buf, pkt_len, &hdr_len, &tag_len, &seq);
	if (ret < 0) {
		return ret;
	}

	if (pkt_len < hdr_len + tag_len + stream->mki_len) {
		return -EBADMSG;
	}

	/* The MKI sits after the encrypted portion, which for the AEAD suites
	 * includes the tag, and before a HMAC authentication tag (RFC 3711
	 * 3.1, RFC 7714 section 4). It is immutable stream state, so it can
	 * be matched before the lock is taken.
	 */
	if (stream->mki_len > 0) {
		size_t mki_off = pkt_len - stream->mki_len -
				 (srtp_cipher_is_aead(stream->cipher) ? 0 : tag_len);

		if (memcmp(&buf[mki_off], stream->mki, stream->mki_len) != 0) {
			NET_DBG("Packet MKI does not match stream");
			return -ENOENT;
		}
	}

	ret = k_mutex_lock(&stream->lock, LOCK_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

	ret = srtp_index_estimate(stream, seq, &v, &index);
	if (ret < 0) {
		goto unlock;
	}

	ret = srtp_replay_check(stream, index);
	if (ret < 0) {
		goto unlock;
	}

	/* The packet index selects the key segment, so keys for the estimated
	 * segment must be derived before the tag can be verified. Derive them
	 * into a candidate set: an unauthenticated packet must not mutate the
	 * stream's key state.
	 */
	k_e = stream->k_e;
	k_a = stream->k_a;
	k_s = stream->k_s;

	if (stream->kdr != 0 && srtp_needs_keys(stream) &&
	    (index / stream->kdr) != stream->key_segment) {
		ret = srtp_keyset_derive(stream, index / stream->kdr, &cand);
		if (ret < 0) {
			goto unlock;
		}

		cand_keys = true;
		k_e = cand.k_e;
		k_a = cand.k_a;
		k_s = cand.k_s;
	}

	switch (stream->cipher) {
	case SRTP_CIPHER_AES_128_GCM:
	case SRTP_CIPHER_AES_256_GCM: {
		uint8_t iv[SRTP_AES_GCM_IV_LEN];

		srtp_gcm_iv(k_s, stream->ssrc, v, seq, iv);
		ret = srtp_crypto_gcm_decrypt(k_e, iv, buf, hdr_len, &buf[hdr_len],
					      pkt_len - stream->mki_len - hdr_len);
		if (ret < 0) {
			goto drop_cand;
		}
		break;
	}
	case SRTP_CIPHER_AES_128_CM:
	case SRTP_CIPHER_NULL:
	default:
		if (stream->auth != SRTP_AUTH_NULL) {
			ret = srtp_crypto_auth_verify(k_a, srtp_auth_tag_len(stream->auth), buf,
						      pkt_len - tag_len - stream->mki_len, v,
						      &buf[pkt_len - tag_len]);
			if (ret < 0) {
				goto drop_cand;
			}
		}

		if (stream->cipher == SRTP_CIPHER_AES_128_CM) {
			uint8_t iv[16];

			srtp_cm_iv(k_s, stream->ssrc, index, iv);
			ret = srtp_crypto_cm_crypt(k_e, iv, &buf[hdr_len],
						   pkt_len - tag_len - stream->mki_len - hdr_len);
			if (ret < 0) {
				goto drop_cand;
			}
		}
		break;
	}

	/* Only update state, including the session keys, once the packet is
	 * authenticated (RFC 3711 3.3.2); keys derived for an older segment
	 * are not committed, keeping the current segment's keys resident.
	 */
	if (cand_keys) {
		if (index / stream->kdr > stream->key_segment) {
			srtp_keyset_commit(stream, &cand, index / stream->kdr);
		} else {
			srtp_keyset_destroy(&cand);
		}
	}

	srtp_replay_advance(stream, index);

	if (index > (((uint64_t)stream->roc << 16) | stream->s_l) || !stream->seq_initialized) {
		stream->roc = v;
		stream->s_l = seq;
		stream->seq_initialized = true;
	}

	*out_len = pkt_len - tag_len - stream->mki_len;
	ret = 0;

	goto unlock;

drop_cand:
	if (cand_keys) {
		srtp_keyset_destroy(&cand);
	}

unlock:
	(void)k_mutex_unlock(&stream->lock);
	return ret;
}
