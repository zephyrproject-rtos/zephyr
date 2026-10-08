/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Kirill Shypachov
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief STUN (Session Traversal Utilities for NAT, RFC 8489) message codec
 */

#ifndef ZEPHYR_INCLUDE_NET_STUN_H_
#define ZEPHYR_INCLUDE_NET_STUN_H_

/**
 * @brief STUN library
 * @defgroup stun STUN (Session Traversal Utilities for NAT)
 * @since 4.6
 * @version 0.1.0
 * @ingroup networking
 *
 * Parsing and building of STUN messages as RFC 8489 defines them, with the
 * attributes of ICE (RFC 8445). The library does no I/O and keeps no state:
 * every function works on buffers of the caller. Message integrity is computed
 * through the PSA Crypto API, which has to be initialised.
 * @{
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/net/net_ip.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Magic cookie, the second 32-bit word of every message (RFC 8489 §5). */
#define STUN_MAGIC_COOKIE 0x2112A442UL
/** Size of the message header. */
#define STUN_HEADER_SIZE  20U
/** Size of the transaction id. */
#define STUN_TXID_SIZE    12U

/** Length of the MESSAGE-INTEGRITY value, an HMAC-SHA1. */
#define STUN_INTEGRITY_LEN            20U
/** Shortest MESSAGE-INTEGRITY-SHA256 value a STUN Usage may allow (RFC 8489 §14.6). */
#define STUN_INTEGRITY_SHA256_MIN_LEN 16U
/** Length of a MESSAGE-INTEGRITY-SHA256 value that is not truncated. */
#define STUN_INTEGRITY_SHA256_MAX_LEN 32U

/**
 * Longest USERNAME, SOFTWARE, REALM, NONCE or ERROR-CODE reason phrase the
 * parser accepts, in bytes (RFC 8489 §14.3, §14.8).
 */
#define STUN_TEXT_MAX_LEN     763U
/** Longest USERNAME the builder sends, in bytes (RFC 8489 §14.3). */
#define STUN_USERNAME_MAX_LEN 508U
/**
 * Longest SOFTWARE and ERROR-CODE reason phrase the builder sends, in UTF-8
 * characters (RFC 8489 §14.8, §14.14).
 */
#define STUN_TEXT_MAX_CHARS   127U

/**
 * @name Message types
 * @{
 */
/** Binding request. */
#define STUN_BINDING_REQUEST          0x0001U
/** Binding success response. */
#define STUN_BINDING_SUCCESS_RESPONSE 0x0101U
/** Binding error response. */
#define STUN_BINDING_ERROR_RESPONSE   0x0111U
/** @} */

/** Binding method (RFC 8489 §18.2). */
#define STUN_METHOD_BINDING 0x001U

/** Message classes (RFC 8489 §5). */
enum stun_class {
	/** Request. */
	STUN_CLASS_REQUEST = 0,
	/** Indication. */
	STUN_CLASS_INDICATION = 1,
	/** Success response. */
	STUN_CLASS_SUCCESS = 2,
	/** Error response. */
	STUN_CLASS_ERROR = 3,
};

/**
 * @name Attribute types the codec understands
 *
 * RFC 8489 §18.3 and RFC 8445 §16.1.
 * @{
 */
/** MAPPED-ADDRESS. */
#define STUN_ATTR_MAPPED_ADDRESS           0x0001U
/** USERNAME. */
#define STUN_ATTR_USERNAME                 0x0006U
/** MESSAGE-INTEGRITY. */
#define STUN_ATTR_MESSAGE_INTEGRITY        0x0008U
/** ERROR-CODE. */
#define STUN_ATTR_ERROR_CODE               0x0009U
/** UNKNOWN-ATTRIBUTES. */
#define STUN_ATTR_UNKNOWN_ATTRIBUTES       0x000AU
/** REALM. */
#define STUN_ATTR_REALM                    0x0014U
/** NONCE. */
#define STUN_ATTR_NONCE                    0x0015U
/** MESSAGE-INTEGRITY-SHA256. */
#define STUN_ATTR_MESSAGE_INTEGRITY_SHA256 0x001CU
/** XOR-MAPPED-ADDRESS. */
#define STUN_ATTR_XOR_MAPPED_ADDRESS       0x0020U
/** PRIORITY (ICE). */
#define STUN_ATTR_PRIORITY                 0x0024U
/** USE-CANDIDATE (ICE). */
#define STUN_ATTR_USE_CANDIDATE            0x0025U
/** SOFTWARE. */
#define STUN_ATTR_SOFTWARE                 0x8022U
/** FINGERPRINT. */
#define STUN_ATTR_FINGERPRINT              0x8028U
/** ICE-CONTROLLED (ICE). */
#define STUN_ATTR_ICE_CONTROLLED           0x8029U
/** ICE-CONTROLLING (ICE). */
#define STUN_ATTR_ICE_CONTROLLING          0x802AU
/** @} */

/**
 * @brief A parsed message
 *
 * The header fields and what the parser found of the attributes it
 * understands. Offsets and pointers refer to the buffer that was parsed, which
 * has to stay as it is for as long as the structure is used.
 */
struct stun_msg {
	/** Message type: method and class. */
	uint16_t type;
	/** Header Length field: the size of the attributes. */
	uint16_t length;
	/** Transaction id. */
	uint8_t txid[STUN_TXID_SIZE];

	/** USERNAME value, not terminated; NULL if absent. */
	const uint8_t *username;
	/** Length of the USERNAME value. */
	uint16_t username_len;

	/** MESSAGE-INTEGRITY is present. */
	bool has_integrity;
	/** Offset of the MESSAGE-INTEGRITY attribute. */
	size_t integrity_off;

	/** MESSAGE-INTEGRITY-SHA256 is present. */
	bool has_integrity_sha256;
	/** Offset of the MESSAGE-INTEGRITY-SHA256 attribute. */
	size_t integrity_sha256_off;
	/** Length of the MESSAGE-INTEGRITY-SHA256 value, 16 to 32. */
	uint16_t integrity_sha256_len;

	/** FINGERPRINT is present. */
	bool has_fingerprint;
	/** Offset of the FINGERPRINT attribute. */
	size_t fingerprint_off;

	/**
	 * Where the reported attributes end: the offset of the first
	 * MESSAGE-INTEGRITY or MESSAGE-INTEGRITY-SHA256, else the end of the
	 * message. What follows is not covered by the integrity check and is
	 * ignored, except FINGERPRINT and a MESSAGE-INTEGRITY-SHA256 that comes
	 * after MESSAGE-INTEGRITY (RFC 8489 §9). A MESSAGE-INTEGRITY placed after
	 * MESSAGE-INTEGRITY-SHA256 is out of order and ignored like the rest.
	 */
	size_t attrs_end;

	/** PRIORITY is present. */
	bool has_priority;
	/** PRIORITY value. */
	uint32_t priority;

	/** USE-CANDIDATE is present. */
	bool has_use_candidate;

	/** ERROR-CODE is present. */
	bool has_error_code;
	/** Error code: class * 100 + number, 300 to 699. The reason phrase is not kept. */
	uint16_t error_code;

	/** ICE-CONTROLLING is present. */
	bool has_ice_controlling;
	/** ICE-CONTROLLED is present. */
	bool has_ice_controlled;
	/** Tie-breaker of whichever role attribute is present. */
	uint64_t ice_tiebreaker;
};

/**
 * @brief Tell whether a datagram is a STUN message
 *
 * Applies the header checks of RFC 8489 §6.3 that need no state: at least a
 * header long, the two top bits zero, the magic cookie, and a Length that is a
 * multiple of 4 and accounts for the whole datagram. Meant for telling STUN
 * from other protocols on a shared port; stun_parse() applies the same checks.
 *
 * @param buf Datagram; NULL is not a message.
 * @param len Its length.
 *
 * @return true if the datagram has the header of a STUN message.
 */
bool stun_is_message(const uint8_t *buf, size_t len);

/**
 * @brief Parse one datagram
 *
 * The parser follows RFC 8489 to the letter:
 * - the message fills the datagram: Length is a multiple of 4 and equals
 *   @p len - 20 (§5, §6.3);
 * - of each attribute type only the first occurrence is processed, later ones
 *   are skipped unexamined (§14);
 * - attributes after MESSAGE-INTEGRITY are ignored, except
 *   MESSAGE-INTEGRITY-SHA256 and FINGERPRINT; without MESSAGE-INTEGRITY the
 *   boundary is MESSAGE-INTEGRITY-SHA256 (§9);
 * - FINGERPRINT is the last attribute (§14.7);
 * - an attribute the codec understands must be well-formed — the lengths and
 *   values the RFCs fix, at most @ref STUN_TEXT_MAX_LEN bytes of text — or the
 *   whole message is malformed;
 * - ICE-CONTROLLING and ICE-CONTROLLED together are malformed (RFC 8445
 *   §7.1.3).
 *
 * Attributes the codec does not understand are skipped; stun_unknown_attrs()
 * reports the comprehension-required ones.
 *
 * @param buf Datagram.
 * @param len Its length.
 * @param out Result; undefined on error.
 *
 * @retval 0 on success.
 * @retval -EINVAL NULL argument, or not STUN at all: shorter than a header,
 *         top bits set, wrong magic cookie.
 * @retval -EBADMSG Malformed message.
 */
int stun_parse(const uint8_t *buf, size_t len, struct stun_msg *out);

/**
 * @brief Compose a message type
 *
 * @param method Method, 12 bits.
 * @param cls Class.
 *
 * @return The 14-bit message type (RFC 8489 §5).
 */
uint16_t stun_msg_type(uint16_t method, enum stun_class cls);

/**
 * @brief Method of a message type
 *
 * @param type Message type.
 *
 * @return The method.
 */
uint16_t stun_msg_method(uint16_t type);

/**
 * @brief Class of a message type
 *
 * @param type Message type.
 *
 * @return The class.
 */
enum stun_class stun_msg_class(uint16_t type);

/**
 * @brief Find an attribute by type
 *
 * Returns the first attribute of the type among the reported ones (see
 * stun_msg::attrs_end), for attributes the codec has no getter for.
 * MESSAGE-INTEGRITY, MESSAGE-INTEGRITY-SHA256 and FINGERPRINT are found where
 * the parser recorded them.
 *
 * @param buf The datagram @p msg was parsed from.
 * @param len Its length.
 * @param msg Result of stun_parse().
 * @param type Attribute type.
 * @param val Receives a pointer to the value, inside @p buf.
 * @param vlen Receives the length of the value, without padding.
 *
 * @retval 0 on success.
 * @retval -ENOENT No such attribute.
 * @retval -EINVAL NULL argument, or @p msg does not belong to @p buf.
 */
int stun_find_attr(const uint8_t *buf, size_t len, const struct stun_msg *msg, uint16_t type,
		   const uint8_t **val, uint16_t *vlen);

/**
 * @brief List the comprehension-required attributes nobody understands
 *
 * These are the attributes with a type of 0x0000 to 0x7FFF that neither the
 * codec understands (MAPPED-ADDRESS, USERNAME, MESSAGE-INTEGRITY, ERROR-CODE,
 * UNKNOWN-ATTRIBUTES, REALM, NONCE, MESSAGE-INTEGRITY-SHA256,
 * XOR-MAPPED-ADDRESS, PRIORITY, USE-CANDIDATE) nor the caller, who lists the
 * further types it handles in @p known. RFC 8489 §6.3: a response with any of
 * them is discarded and its transaction has failed, a request is answered with
 * 420 and UNKNOWN-ATTRIBUTES.
 *
 * @param buf The datagram @p msg was parsed from.
 * @param len Its length.
 * @param msg Result of stun_parse().
 * @param known Further attribute types the caller understands; may be NULL if
 *        @p n_known is 0.
 * @param n_known Number of entries in @p known.
 * @param out Receives the types, in order of appearance, each once; may be
 *        NULL if @p cap is 0.
 * @param cap Room in @p out.
 *
 * @return The number of distinct unknown types, which may exceed @p cap.
 * @retval -EINVAL NULL @p buf or @p msg, a NULL list with a non-zero count, or
 *         @p msg does not belong to @p buf.
 */
int stun_unknown_attrs(const uint8_t *buf, size_t len, const struct stun_msg *msg,
		       const uint16_t *known, size_t n_known, uint16_t *out, size_t cap);

/**
 * @brief Verify MESSAGE-INTEGRITY
 *
 * Recomputes the HMAC-SHA1 of RFC 8489 §14.5 and compares it in constant time.
 * The key is the password for short-term credentials.
 *
 * @param buf The datagram @p msg was parsed from.
 * @param len Its length.
 * @param msg Result of stun_parse().
 * @param key Key.
 * @param key_len Its length, not 0.
 *
 * @retval 0 The message is authentic.
 * @retval -ENOENT No MESSAGE-INTEGRITY attribute.
 * @retval -EBADMSG The value does not match.
 * @retval -EINVAL NULL argument, empty key, or @p msg does not belong to @p buf.
 * @retval -EIO The cryptographic backend failed.
 */
int stun_verify_integrity(const uint8_t *buf, size_t len, const struct stun_msg *msg,
			  const uint8_t *key, size_t key_len);

/**
 * @brief Verify MESSAGE-INTEGRITY-SHA256
 *
 * Recomputes the HMAC-SHA256 of RFC 8489 §14.6 and compares it in constant
 * time. The value has to be the full 32 bytes unless the STUN Usage allows
 * truncation, which the caller states with @p min_len.
 *
 * @param buf The datagram @p msg was parsed from.
 * @param len Its length.
 * @param msg Result of stun_parse().
 * @param key Key.
 * @param key_len Its length, not 0.
 * @param min_len Shortest value to accept: 32, or a multiple of 4 not below 16.
 *
 * @retval 0 The message is authentic.
 * @retval -ENOENT No MESSAGE-INTEGRITY-SHA256 attribute.
 * @retval -EBADMSG The value does not match, or is shorter than @p min_len.
 * @retval -EINVAL NULL argument, empty key, @p min_len out of range, or
 *         @p msg does not belong to @p buf.
 * @retval -EIO The cryptographic backend failed.
 */
int stun_verify_integrity_sha256(const uint8_t *buf, size_t len, const struct stun_msg *msg,
				 const uint8_t *key, size_t key_len, size_t min_len);

/**
 * @brief Verify FINGERPRINT
 *
 * @param buf The datagram @p msg was parsed from.
 * @param len Its length.
 * @param msg Result of stun_parse().
 *
 * @retval 0 The fingerprint is correct.
 * @retval -ENOENT No FINGERPRINT attribute.
 * @retval -EBADMSG The value does not match.
 * @retval -EINVAL NULL argument, or @p msg does not belong to @p buf.
 */
int stun_verify_fingerprint(const uint8_t *buf, size_t len, const struct stun_msg *msg);

/**
 * @brief Decode an attribute in the XOR-MAPPED-ADDRESS format
 *
 * For XOR-MAPPED-ADDRESS itself use stun_get_xor_mapped_address(); this is for
 * the attributes of STUN extensions that share its format.
 *
 * @param buf The datagram @p msg was parsed from.
 * @param len Its length.
 * @param msg Result of stun_parse().
 * @param attr_type Attribute type.
 * @param addr Receives the address and port.
 *
 * @retval 0 on success.
 * @retval -ENOENT No such attribute.
 * @retval -EBADMSG The attribute is malformed.
 * @retval -EAFNOSUPPORT An IPv6 address, and the IPv6 stack is not enabled.
 * @retval -EINVAL NULL argument, or @p msg does not belong to @p buf.
 */
int stun_get_xor_address(const uint8_t *buf, size_t len, const struct stun_msg *msg,
			 uint16_t attr_type, struct net_sockaddr *addr);

/**
 * @brief Decode XOR-MAPPED-ADDRESS
 *
 * @param buf The datagram @p msg was parsed from.
 * @param len Its length.
 * @param msg Result of stun_parse().
 * @param addr Receives the address and port.
 *
 * @retval 0 on success.
 * @retval -ENOENT No XOR-MAPPED-ADDRESS attribute.
 * @retval -EAFNOSUPPORT An IPv6 address, and the IPv6 stack is not enabled.
 * @retval -EINVAL NULL argument, or @p msg does not belong to @p buf.
 */
int stun_get_xor_mapped_address(const uint8_t *buf, size_t len, const struct stun_msg *msg,
				struct net_sockaddr *addr);

/**
 * @brief A message under construction
 *
 * Set up with stun_builder_init(). The header in the buffer is valid after
 * every attribute added, so the message can be sent as soon as
 * stun_builder_len() or one of the stun_finish functions has returned its
 * length.
 *
 * The functions that add to it return 0 or a negative errno. Running out of
 * room (-EMSGSIZE) and a failure of the cryptographic backend (-EIO) are
 * remembered: every later call returns the same error. A refused argument
 * (-EINVAL, -EAFNOSUPPORT) or a call out of order (-EPERM) leaves the message
 * as it was and may be followed by further calls.
 */
struct stun_builder {
	/** @cond INTERNAL_HIDDEN */
	uint8_t *buf;
	size_t cap;
	size_t len;
	int err;
	uint8_t stage;
	/** @endcond */
};

/**
 * @brief Start a message
 *
 * @param b Builder.
 * @param buf Buffer for the message.
 * @param cap Its size.
 * @param msg_type Message type.
 * @param txid Transaction id. For a request it has to come from a
 *        cryptographically secure random generator (RFC 8489 §5).
 */
void stun_builder_init(struct stun_builder *b, uint8_t *buf, size_t cap, uint16_t msg_type,
		       const uint8_t txid[STUN_TXID_SIZE]);

/**
 * @brief Add an attribute by type and value
 *
 * For the attributes of STUN extensions. MESSAGE-INTEGRITY,
 * MESSAGE-INTEGRITY-SHA256 and FINGERPRINT have functions of their own and are
 * refused here.
 *
 * @param b Builder.
 * @param type Attribute type.
 * @param val Value; may be NULL if @p vlen is 0.
 * @param vlen Its length, at most 65535.
 *
 * @return 0 or a negative errno, see @ref stun_builder.
 */
int stun_add_attr(struct stun_builder *b, uint16_t type, const uint8_t *val, size_t vlen);

/**
 * @brief Add SOFTWARE
 *
 * @param b Builder.
 * @param sw Description of the software, at most @ref STUN_TEXT_MAX_CHARS
 *        UTF-8 characters.
 *
 * @return 0 or a negative errno, see @ref stun_builder.
 */
int stun_add_software(struct stun_builder *b, const char *sw);

/**
 * @brief Add USERNAME
 *
 * @param b Builder.
 * @param username User name, at most @ref STUN_USERNAME_MAX_LEN bytes, already
 *        processed with the OpaqueString profile (RFC 8265).
 *
 * @return 0 or a negative errno, see @ref stun_builder.
 */
int stun_add_username(struct stun_builder *b, const char *username);

/**
 * @brief Add an attribute in the XOR-MAPPED-ADDRESS format
 *
 * @param b Builder.
 * @param attr_type Attribute type.
 * @param addr IPv4 or IPv6 address and port.
 *
 * @return 0 or a negative errno, see @ref stun_builder; -EAFNOSUPPORT for an
 *         address family other than IPv4 and IPv6, and for IPv6 when the IPv6
 *         stack is not enabled.
 */
int stun_add_xor_address(struct stun_builder *b, uint16_t attr_type,
			 const struct net_sockaddr *addr);

/**
 * @brief Add XOR-MAPPED-ADDRESS
 *
 * @param b Builder.
 * @param addr IPv4 or IPv6 address and port.
 *
 * @return 0 or a negative errno, see stun_add_xor_address().
 */
int stun_add_xor_mapped_address(struct stun_builder *b, const struct net_sockaddr *addr);

/**
 * @brief Add PRIORITY
 *
 * @param b Builder.
 * @param priority Priority of the candidate (RFC 8445 §7.1.1).
 *
 * @return 0 or a negative errno, see @ref stun_builder.
 */
int stun_add_priority(struct stun_builder *b, uint32_t priority);

/**
 * @brief Add USE-CANDIDATE
 *
 * @param b Builder.
 *
 * @return 0 or a negative errno, see @ref stun_builder.
 */
int stun_add_use_candidate(struct stun_builder *b);

/**
 * @brief Add ERROR-CODE
 *
 * @param b Builder.
 * @param code Error code, 300 to 699.
 * @param reason Reason phrase, at most @ref STUN_TEXT_MAX_CHARS UTF-8
 *        characters; NULL for none.
 *
 * @return 0 or a negative errno, see @ref stun_builder.
 */
int stun_add_error_code(struct stun_builder *b, uint16_t code, const char *reason);

/**
 * @brief Add UNKNOWN-ATTRIBUTES
 *
 * The attribute a 420 error response carries (RFC 8489 §6.3.1); the list is
 * what stun_unknown_attrs() returned for the request.
 *
 * @param b Builder.
 * @param types Attribute types; may be NULL if @p n is 0.
 * @param n Number of types.
 *
 * @return 0 or a negative errno, see @ref stun_builder.
 */
int stun_add_unknown_attributes(struct stun_builder *b, const uint16_t *types, size_t n);

/**
 * @brief Add ICE-CONTROLLING
 *
 * @param b Builder.
 * @param tiebreaker Tie-breaker of the agent (RFC 8445 §7.1.3).
 *
 * @return 0 or a negative errno, see @ref stun_builder.
 */
int stun_add_ice_controlling(struct stun_builder *b, uint64_t tiebreaker);

/**
 * @brief Add ICE-CONTROLLED
 *
 * @param b Builder.
 * @param tiebreaker Tie-breaker of the agent (RFC 8445 §7.1.3).
 *
 * @return 0 or a negative errno, see @ref stun_builder.
 */
int stun_add_ice_controlled(struct stun_builder *b, uint64_t tiebreaker);

/**
 * @brief Add MESSAGE-INTEGRITY
 *
 * Signs what the message holds so far with HMAC-SHA1 (RFC 8489 §14.5). After
 * it only MESSAGE-INTEGRITY-SHA256 and FINGERPRINT can be added.
 *
 * @param b Builder.
 * @param key Key; the password for short-term credentials.
 * @param key_len Its length, not 0.
 *
 * @return 0 or a negative errno, see @ref stun_builder.
 */
int stun_add_integrity(struct stun_builder *b, const uint8_t *key, size_t key_len);

/**
 * @brief Add MESSAGE-INTEGRITY-SHA256
 *
 * Signs what the message holds so far with HMAC-SHA256, not truncated (RFC
 * 8489 §14.6). After it only FINGERPRINT can be added. RFC 8489 §9.1.2: a
 * request carries both integrity attributes unless the two agents know by
 * other means which one they share, as ICE agents do.
 *
 * @param b Builder.
 * @param key Key; the password for short-term credentials.
 * @param key_len Its length, not 0.
 *
 * @return 0 or a negative errno, see @ref stun_builder.
 */
int stun_add_integrity_sha256(struct stun_builder *b, const uint8_t *key, size_t key_len);

/**
 * @brief Add FINGERPRINT
 *
 * The last attribute of a message (RFC 8489 §14.7): nothing can be added after
 * it.
 *
 * @param b Builder.
 *
 * @return 0 or a negative errno, see @ref stun_builder.
 */
int stun_add_fingerprint(struct stun_builder *b);

/**
 * @brief Length of the message built so far
 *
 * @param b Builder.
 *
 * @return The length of the message in the buffer, or the error the builder
 *         remembers.
 */
int stun_builder_len(const struct stun_builder *b);

/**
 * @brief Close a message with MESSAGE-INTEGRITY and FINGERPRINT
 *
 * The same as stun_add_integrity(), stun_add_fingerprint() and
 * stun_builder_len(): what an ICE agent ends its messages with.
 *
 * @param b Builder.
 * @param key Key; the password for short-term credentials.
 * @param key_len Its length, not 0.
 *
 * @return The length of the message, or a negative errno, see @ref stun_builder.
 */
int stun_finish(struct stun_builder *b, const uint8_t *key, size_t key_len);

/**
 * @brief Close a message with FINGERPRINT alone
 *
 * The same as stun_add_fingerprint() and stun_builder_len(), for messages that
 * are not authenticated.
 *
 * @param b Builder.
 *
 * @return The length of the message, or a negative errno, see @ref stun_builder.
 */
int stun_finish_plain(struct stun_builder *b);

/**
 * @name Binding transaction
 *
 * One Binding request to a STUN server and its response, which tells the
 * transport address the server sees the request come from (RFC 8489 §6, §12).
 * The transaction does no I/O and reads no clock: the caller sends what it is
 * told to send, feeds in what it receives, and says what time it is.
 * @{
 */

/** Size of the buffer a transaction keeps its request in. */
#define STUN_BINDING_REQUEST_SIZE 128U

/** Retransmission timing of a transaction (RFC 8489 §6.2.1). */
struct stun_txn_policy {
	/** Initial retransmission timeout (RTO) in milliseconds, 1 to 60000. */
	uint32_t rto_ms;
	/** Rc: the number of transmissions, the first included, 1 to 16. */
	uint8_t max_transmits;
	/** Rm: the wait after the last transmission, in RTOs, at least 1. */
	uint8_t final_wait_rto;
};

/** What the caller of a transaction has to do next. */
enum stun_txn_action {
	/** Nothing: the call was refused. */
	STUN_TXN_NONE = 0,
	/** Send stun_txn_step::tx to the server, then call stun_binding_advance() again. */
	STUN_TXN_SEND,
	/** Nothing to do before stun_txn_step::wait_until_ms. */
	STUN_TXN_WAIT,
	/** The transaction has succeeded. */
	STUN_TXN_DONE,
	/** The transaction has failed. */
	STUN_TXN_FAILED,
};

/** One step of a transaction, filled in for the caller. */
struct stun_txn_step {
	/** What to do. */
	enum stun_txn_action action;
	/** The datagram to send, owned by the transaction; set with @ref STUN_TXN_SEND. */
	const uint8_t *tx;
	/** Its length. */
	size_t tx_len;
	/** When to call again, on the caller's millisecond clock; set with @ref STUN_TXN_WAIT. */
	uint32_t wait_until_ms;
};

/** State of a Binding transaction. */
enum stun_binding_state {
	/** Not started. */
	STUN_BINDING_IDLE = 0,
	/** Request sent, no answer yet. */
	STUN_BINDING_IN_FLIGHT,
	/** Answered: stun_binding::mapped is valid. */
	STUN_BINDING_DONE,
	/** Failed: see stun_binding::fail. */
	STUN_BINDING_FAILED,
};

/** Why a Binding transaction failed. */
enum stun_binding_fail {
	/** It has not failed. */
	STUN_BINDING_FAIL_NONE = 0,
	/** The server did not answer. */
	STUN_BINDING_FAIL_TIMEOUT,
	/** The server answered with an error response; see stun_binding::error_code. */
	STUN_BINDING_FAIL_ERROR_RESPONSE,
	/**
	 * The server's response cannot be used: it has a comprehension-required
	 * attribute nobody understands, or no address this build can represent.
	 */
	STUN_BINDING_FAIL_BAD_RESPONSE,
	/** stun_binding_cancel() was called. */
	STUN_BINDING_FAIL_CANCELLED,
};

/**
 * @brief A Binding transaction
 *
 * Zero it before the first stun_binding_start(). The members are for reading.
 */
struct stun_binding {
	/** State. */
	enum stun_binding_state state;
	/** Why it failed, in @ref STUN_BINDING_FAILED. */
	enum stun_binding_fail fail;
	/** Error code of the server's error response, 0 if it carried none. */
	uint16_t error_code;
	/** The address the server sees, in @ref STUN_BINDING_DONE. */
	struct net_sockaddr mapped;
	/** Datagrams offered that were not believed to be the server's answer. */
	uint32_t rx_rejected;

	/** @cond INTERNAL_HIDDEN */
	struct net_sockaddr server;
	struct stun_txn_policy policy;
	uint8_t req[STUN_BINDING_REQUEST_SIZE];
	size_t req_len;
	uint32_t deadline_ms;
	uint8_t attempt;
	/** @endcond */
};

/**
 * @brief Start a Binding transaction
 *
 * Builds the request — with SOFTWARE if asked for, and FINGERPRINT — and
 * returns its first transmission in @p step.
 *
 * @param b Transaction; not one that is in flight.
 * @param server Address and port of the STUN server.
 * @param txid Transaction id, from a cryptographically secure random
 *        generator (RFC 8489 §5).
 * @param software Value of the SOFTWARE attribute, which a request should
 *        carry (RFC 8489 §6.1); NULL to send none.
 * @param policy Retransmission timing; NULL for the defaults of RFC 8489:
 *        RTO 500 ms, 7 transmissions, a final wait of 16 RTO.
 * @param now_ms Current time on a millisecond clock of the caller's choice,
 *        which may wrap around.
 * @param step Receives the first step, @ref STUN_TXN_SEND.
 *
 * @retval 0 on success.
 * @retval -EINVAL NULL argument, a server without a port, a policy out of
 *         range, or a @p software the SOFTWARE attribute cannot carry.
 * @retval -EAFNOSUPPORT The server address is neither IPv4 nor IPv6, or it is
 *         IPv6 and the IPv6 stack is not enabled.
 * @retval -EMSGSIZE @p software does not fit the request buffer.
 * @retval -EALREADY The transaction is in flight.
 */
int stun_binding_start(struct stun_binding *b, const struct net_sockaddr *server,
		       const uint8_t txid[STUN_TXID_SIZE], const char *software,
		       const struct stun_txn_policy *policy, uint32_t now_ms,
		       struct stun_txn_step *step);

/**
 * @brief Let time pass in a Binding transaction
 *
 * Call it after sending what a step asked for, and when the time a
 * @ref STUN_TXN_WAIT step named has come. Retransmissions follow the schedule
 * of RFC 8489 §6.2.1; each interval is counted from the transmission before
 * it, so a call that comes late moves the rest of the schedule with it.
 *
 * @param b Transaction.
 * @param now_ms Current time, on the clock given to stun_binding_start().
 * @param step Receives the next step.
 *
 * @retval 0 on success.
 * @retval -EINVAL NULL argument.
 * @retval -EPERM The transaction was never started.
 */
int stun_binding_advance(struct stun_binding *b, uint32_t now_ms, struct stun_txn_step *step);

/**
 * @brief Tell whether a datagram answers a Binding transaction
 *
 * A cheap check for routing on a socket shared with other traffic: a STUN
 * message that is a Binding response and carries the transaction id.
 *
 * @param b Transaction.
 * @param buf Datagram.
 * @param len Its length.
 *
 * @return true if the datagram is a response to this transaction's request.
 */
bool stun_binding_owns(const struct stun_binding *b, const uint8_t *buf, size_t len);

/**
 * @brief Offer a received datagram to a Binding transaction
 *
 * The datagram is taken for the server's answer only if it is a Binding
 * response with the transaction id, comes from the server's address and port,
 * is well-formed, and has a correct FINGERPRINT if it has one. Anything else
 * is counted in stun_binding::rx_rejected and the transaction goes on waiting:
 * a datagram from elsewhere can neither end it nor plant an address.
 *
 * The server's answer ends the transaction, in @ref STUN_BINDING_DONE or in
 * @ref STUN_BINDING_FAILED (RFC 8489 §6.3.3, §6.3.4).
 *
 * @param b Transaction.
 * @param from Where the datagram came from.
 * @param buf Datagram.
 * @param len Its length.
 *
 * @retval 0 The datagram was the server's answer; see stun_binding::state.
 * @retval -ENOENT Not a response to this transaction.
 * @retval -EACCES Not from the server.
 * @retval -EBADMSG Malformed, or its FINGERPRINT is wrong.
 * @retval -EALREADY The transaction is not in flight.
 * @retval -EINVAL NULL argument.
 */
int stun_binding_on_datagram(struct stun_binding *b, const struct net_sockaddr *from,
			     const uint8_t *buf, size_t len);

/**
 * @brief Give up on a Binding transaction
 *
 * A transaction in flight fails with @ref STUN_BINDING_FAIL_CANCELLED; in any
 * other state nothing happens.
 *
 * @param b Transaction.
 */
void stun_binding_cancel(struct stun_binding *b);

/** @} */

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_NET_STUN_H_ */
