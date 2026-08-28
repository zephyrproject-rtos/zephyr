/*
 *  SPDX-FileCopyrightText: 2026 Basalte bv
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SRTP (Secure Real-time Transport Protocol) API
 */

#ifndef ZEPHYR_INCLUDE_NET_SRTP_H_
#define ZEPHYR_INCLUDE_NET_SRTP_H_

#include <psa/crypto.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup srtp SRTP (Secure Real-time Transport Protocol)
 * @version 0.1.0
 * @since 4.5
 * @ingroup networking
 * @{
 *
 * SRTP packet protection as defined in @rfc{3711}, with AES-GCM crypto suites
 * as defined in @rfc{7714}. Master keys are provisioned by the application;
 * key exchange (DTLS-SRTP, SDES, MIKEY) and SRTCP are out of scope.
 */

/** Length of an AES-128 key in bytes. */
#define SRTP_AES_128_KEY_LEN   16
/** Length of an AES-256 key in bytes. */
#define SRTP_AES_256_KEY_LEN   32
/** Length of the SRTP master and session salt in bytes (@rfc{3711}). */
#define SRTP_MASTER_SALT_LEN   14
/** Length of the SRTP master and session salt in bytes for AES-GCM (@rfc{7714}). */
#define SRTP_AES_GCM_SALT_LEN  12
/** Length of a derived HMAC-SHA1 session authentication key in bytes. */
#define SRTP_HMAC_SHA1_KEY_LEN 20
/** Length of the AES-GCM authentication tag in bytes (@rfc{7714}). */
#define SRTP_AES_GCM_TAG_LEN   16
/** Length of the AES-GCM initialization vector in bytes (@rfc{7714}). */
#define SRTP_AES_GCM_IV_LEN    12
/** Maximum length of a master key identifier (MKI) in bytes. */
#define SRTP_MKI_MAX_LEN       16

/** Maximum number of bytes @ref srtp_protect appends to a packet: the
 *  authentication tag plus the MKI, when configured.
 */
#define SRTP_MAX_TRAILER_LEN (SRTP_AES_GCM_TAG_LEN + SRTP_MKI_MAX_LEN)

/** SRTP encryption transform. */
enum srtp_cipher {
	/** No encryption (@rfc{3711,section-4.1.3}); packets are authenticated only. */
	SRTP_CIPHER_NULL,
	/** AES-128 in counter mode (AES-CM, @rfc{3711,section-4.1.1}). */
	SRTP_CIPHER_AES_128_CM,
	/** AES-128 in Galois/counter mode (@rfc{7714}). Authentication is part of
	 *  the AEAD transform; the policy auth transform must be
	 *  @ref SRTP_AUTH_NULL.
	 */
	SRTP_CIPHER_AES_128_GCM,
	/** AES-256 in Galois/counter mode (@rfc{7714}); session keys are
	 *  derived via the AES_256_CM_PRF (@rfc{6188}). The policy auth
	 *  transform must be @ref SRTP_AUTH_NULL.
	 */
	SRTP_CIPHER_AES_256_GCM,
};

/** SRTP authentication transform. */
enum srtp_auth {
	/** No authentication tag. Insecure for AES-CM and NULL ciphers; requires
	 *  explicit opt-in via srtp_policy.allow_null_auth. Mandatory for the
	 *  AES-GCM ciphers, where the AEAD provides authentication.
	 */
	SRTP_AUTH_NULL,
	/** HMAC-SHA1 with an 80-bit (10-byte) tag (@rfc{3711,section-4.2}). */
	SRTP_AUTH_HMAC_SHA1_80,
	/** HMAC-SHA1 with a 32-bit (4-byte) tag (@rfc{3711,section-4.2}). */
	SRTP_AUTH_HMAC_SHA1_32,
};

/** SRTP stream protection policy. */
struct srtp_policy {
	/** Encryption transform. */
	enum srtp_cipher cipher;
	/** Authentication transform. */
	enum srtp_auth auth;

	/** Master key; @ref SRTP_AES_128_KEY_LEN bytes, or
	 *  @ref SRTP_AES_256_KEY_LEN bytes for @ref SRTP_CIPHER_AES_256_GCM.
	 */
	const uint8_t *master_key;
	/** Length of @p master_key in bytes. */
	size_t master_key_len;
	/** Master salt; @ref SRTP_MASTER_SALT_LEN bytes, or
	 *  @ref SRTP_AES_GCM_SALT_LEN bytes for the AES-GCM ciphers.
	 */
	const uint8_t *master_salt;
	/** Length of @p master_salt in bytes. */
	size_t master_salt_len;

	/** Key derivation rate (@rfc{3711,section-4.3.1}); 0 to derive session
	 *  keys once, otherwise a power of two between 2^0 and 2^24.
	 */
	uint32_t kdr;

	/** Master key identifier (MKI, @rfc{3711,section-3.1}) appended to
	 *  every protected packet and matched on received packets; NULL for
	 *  no MKI. At most @ref SRTP_MKI_MAX_LEN bytes. One master key per
	 *  stream is supported: the MKI tags and matches packets, it does not
	 *  select among multiple keys.
	 */
	const uint8_t *mki;
	/** Length of @p mki in bytes; 0 for no MKI. */
	size_t mki_len;

	/** Explicit opt-in for @ref SRTP_AUTH_NULL with non-AEAD ciphers.
	 *  Without authentication, SRTP provides no integrity or replay
	 *  protection.
	 */
	bool allow_null_auth;
};

/** @cond INTERNAL_HIDDEN */
/** Number of bits per replay window bitmap word */
#define SRTP_REPLAY_WINDOW_WORD_BITS 64
#define SRTP_REPLAY_WINDOW_WORDS                                                                   \
	DIV_ROUND_UP(CONFIG_SRTP_REPLAY_WINDOW_SIZE, SRTP_REPLAY_WINDOW_WORD_BITS)
/** @endcond */

/**
 * SRTP per-SSRC cryptographic context.
 *
 * All members are internal; initialize with @ref srtp_stream_init and release
 * with @ref srtp_stream_deinit. @ref srtp_protect and @ref srtp_unprotect
 * serialize on an internal lock; the strictly increasing transmit order
 * required by @ref srtp_protect must be ensured by the caller.
 */
struct srtp_stream {
	/** @cond INTERNAL_HIDDEN */
	uint32_t ssrc;
	enum srtp_cipher cipher;
	enum srtp_auth auth;
	uint32_t kdr;

	/** Rollover counter (@rfc{3711,section-3.2.1}). */
	uint32_t roc;
	/** Highest received/sent RTP sequence number. */
	uint16_t s_l;
	/** Whether s_l has been initialized from the first packet. */
	bool seq_initialized;
	/** Highest authenticated 48-bit packet index. */
	uint64_t index_max;
	/** Replay protection window bitmap; bit n = index_max - n seen. */
	uint64_t replay_window[SRTP_REPLAY_WINDOW_WORDS];
	/** Number of packets protected with the current master key. */
	uint64_t tx_count;

	/** Master key handle, kept for (re-)derivation of session keys. */
	psa_key_id_t master_key;
	uint8_t master_salt[SRTP_MASTER_SALT_LEN];
	size_t master_salt_len;
	/** Master key identifier carried in protected packets. */
	uint8_t mki[SRTP_MKI_MAX_LEN];
	size_t mki_len;
	/** index DIV kdr for which the session keys are derived. */
	uint64_t key_segment;

	/** Session encryption key handle. */
	psa_key_id_t k_e;
	/** Session authentication key handle. */
	psa_key_id_t k_a;
	/** Session salt. */
	uint8_t k_s[SRTP_MASTER_SALT_LEN];

	struct k_mutex lock;
	/** @endcond */
};

/**
 * @brief Initialize an SRTP stream from a policy.
 *
 * Imports the master key, derives the session keys, and prepares the replay
 * protection state. The policy key material is copied into PSA key storage
 * and the stream; the policy structure itself is not referenced afterwards.
 *
 * @param stream Stream to initialize.
 * @param policy Protection policy and master key material.
 * @param ssrc   Synchronization source identifier of the stream.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure.
 */
int srtp_stream_init(struct srtp_stream *stream, const struct srtp_policy *policy, uint32_t ssrc);

/**
 * @brief Release an SRTP stream.
 *
 * Destroys all PSA keys held by the stream and zeroizes key material.
 *
 * @param stream Stream to release.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure.
 */
int srtp_stream_deinit(struct srtp_stream *stream);

/**
 * @brief Protect a serialized RTP packet in place (@rfc{3711,section-3.3}).
 *
 * Encrypts the payload (including any RTP padding) and appends the MKI, when
 * configured, and the authentication tag. @p buf_size must allow for up to
 * @ref SRTP_MAX_TRAILER_LEN bytes of growth beyond @p pkt_len.
 *
 * Calls for the same stream must be serialized by the caller and made with
 * strictly increasing RTP sequence numbers.
 *
 * @param stream       SRTP stream matching the packet's SSRC.
 * @param buf          Buffer holding a serialized RTP packet.
 * @param pkt_len      Length of the RTP packet in bytes.
 * @param buf_size     Size of @p buf in bytes.
 * @param[out] out_len Length of the protected packet in bytes.
 *
 * @retval 0            On success.
 * @retval -EKEYEXPIRED When 2^48 packets have been protected with this key.
 * @retval negative     Other errno value on failure.
 */
int srtp_protect(struct srtp_stream *stream, uint8_t *buf, size_t pkt_len, size_t buf_size,
		 size_t *out_len);

/**
 * @brief Validate an SRTP policy.
 *
 * @param policy Policy to validate.
 *
 * @retval 0       When the policy is consistent and complete.
 * @retval -EINVAL Otherwise.
 */
int srtp_policy_validate(const struct srtp_policy *policy);

/**
 * @brief Unprotect a serialized SRTP packet in place (@rfc{3711,section-3.3}).
 *
 * Verifies the authentication tag, checks for replay, and decrypts the
 * payload in place. State is only advanced after successful authentication.
 *
 * @param stream       SRTP stream matching the packet's SSRC.
 * @param buf          Buffer holding a serialized SRTP packet.
 * @param pkt_len      Length of the SRTP packet in bytes.
 * @param[out] out_len Length of the plain RTP packet in bytes.
 *
 * @retval 0        On success.
 * @retval -EBADMSG When authentication fails or the packet is malformed.
 * @retval -EALREADY When the packet is a replay.
 * @retval -ENOENT  When the packet's MKI does not match the stream's.
 * @retval negative Other errno value on failure.
 */
int srtp_unprotect(struct srtp_stream *stream, uint8_t *buf, size_t pkt_len, size_t *out_len);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_SRTP_H_ */
