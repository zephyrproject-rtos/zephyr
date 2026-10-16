/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Kirill Shypachov
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief STUN (Session Traversal Utilities for NAT, @rfc{8489}) message codec
 * @ingroup stun
 */

#ifndef ZEPHYR_INCLUDE_NET_STUN_H_
#define ZEPHYR_INCLUDE_NET_STUN_H_

/**
 * @brief Message codec for STUN (@rfc{8489}) with the attributes of ICE (@rfc{8445})
 * @defgroup stun STUN (Session Traversal Utilities for NAT)
 * @since 4.6
 * @version 0.1.0
 * @ingroup networking
 *
 * Parsing and building of STUN messages as @rfc{8489} defines them, with the
 * attributes of ICE (@rfc{8445}). The library does no I/O and keeps no state:
 * every function works on buffers of the caller. Message integrity is computed
 * through the PSA Crypto API, which the library initialises itself.
 * @{
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/net/net_ip.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Magic cookie, the second 32-bit word of every message (@rfc{8489,section-5}). */
#define STUN_MAGIC_COOKIE 0x2112A442UL
/** Size of the message header. */
#define STUN_HEADER_SIZE  20U
/** Size of the transaction id. */
#define STUN_TXID_SIZE    12U

/** Length of the MESSAGE-INTEGRITY value, an HMAC-SHA1. */
#define STUN_INTEGRITY_LEN            20U
/** Shortest MESSAGE-INTEGRITY-SHA256 value a STUN Usage may allow (@rfc{8489,section-14.6}). */
#define STUN_INTEGRITY_SHA256_MIN_LEN 16U
/** Length of a MESSAGE-INTEGRITY-SHA256 value that is not truncated. */
#define STUN_INTEGRITY_SHA256_MAX_LEN 32U

/**
 * Longest USERNAME, SOFTWARE, REALM, NONCE or ERROR-CODE reason phrase the
 * parser accepts, in bytes (@rfc{8489,section-14.3}, @rfc{8489,section-14.8}).
 */
#define STUN_TEXT_MAX_LEN     763U
/** Longest USERNAME the builder sends, in bytes (@rfc{8489,section-14.3}). */
#define STUN_USERNAME_MAX_LEN 508U
/**
 * Longest SOFTWARE and ERROR-CODE reason phrase the builder sends, in UTF-8
 * characters (@rfc{8489,section-14.8}, @rfc{8489,section-14.14}).
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

/** Binding method (@rfc{8489,section-18.2}). */
#define STUN_METHOD_BINDING 0x001U

/** Message classes (@rfc{8489,section-5}). */
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
 * @rfc{8489,section-18.3} and @rfc{8445,section-20.1}.
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

	/**
	 * USERNAME value as received: not terminated and not validated, the
	 * parser checks its length, not its encoding; NULL if absent.
	 */
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
	 * after MESSAGE-INTEGRITY (@rfc{8489,section-9}). A MESSAGE-INTEGRITY placed after
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
 * @brief Check that a datagram starts with a valid STUN header
 *
 * Applies the header checks of @rfc{8489,section-6.3} that need no state: at
 * least a header long, the two top bits zero, the magic cookie, and a Length
 * that is a multiple of 4 and accounts for the whole datagram. Meant for
 * telling STUN from other protocols on a shared port. The attributes are not
 * looked at: stun_parse() applies the same checks first and may still refuse
 * the message.
 *
 * @param buf Datagram; NULL has no valid header.
 * @param len Its length.
 *
 * @return true if the datagram has a valid STUN header.
 */
bool stun_has_valid_header(const uint8_t *buf, size_t len);

/**
 * @brief Parse one datagram
 *
 * The parser applies these rules, from @rfc{8489} and, for ICE, @rfc{8445};
 * where an RFC leaves the choice open, it takes the stricter one:
 * - the message fills the datagram: Length is a multiple of 4 and equals
 *   @p len - 20 (@rfc{8489,section-5}, @rfc{8489,section-6.3});
 * - of each attribute type only the first occurrence is processed, later ones
 *   are skipped unexamined (@rfc{8489,section-14});
 * - attributes after MESSAGE-INTEGRITY are ignored, except
 *   MESSAGE-INTEGRITY-SHA256 and FINGERPRINT; without MESSAGE-INTEGRITY the
 *   boundary is MESSAGE-INTEGRITY-SHA256 (@rfc{8489,section-9});
 * - FINGERPRINT is the last attribute (@rfc{8489,section-14.7});
 * - an attribute the codec understands must be well-formed, or the whole
 *   message is malformed: it has the lengths and values the RFCs fix and at most
 *   @ref STUN_TEXT_MAX_LEN bytes of text, whose encoding is not checked;
 * - an address attribute of a family other than IPv4 and IPv6 is not judged:
 *   the getter reports it (@rfc{8489,section-6.3.3});
 * - ICE-CONTROLLING and ICE-CONTROLLED together are malformed: an agent sends
 *   the attribute of its one role (@rfc{8445,section-7.1.3}), never both.
 *
 * Attributes the codec does not understand are skipped; stun_unknown_attrs()
 * reports the comprehension-required ones.
 *
 * @param buf Datagram.
 * @param len Its length.
 * @param[out] out Result; undefined on error.
 *
 * @retval 0 Success.
 * @retval -EINVAL NULL argument, or not STUN at all: shorter than a header,
 *         top bits set, wrong magic cookie.
 * @retval -EBADMSG Malformed message.
 */
int stun_parse(const uint8_t *buf, size_t len, struct stun_msg *out);

/**
 * @brief Compose a message type
 *
 * @param method Method, 12 bits.
 * @param msg_class Message class: request, indication, success or error response.
 *
 * @return The 14-bit message type (@rfc{8489,section-5}).
 */
uint16_t stun_msg_type_compose(uint16_t method, enum stun_class msg_class);

/**
 * @brief Get the method of a message type
 *
 * @param type Message type.
 *
 * @return The method.
 */
uint16_t stun_msg_type_get_method(uint16_t type);

/**
 * @brief Get the class of a message type
 *
 * @param type Message type.
 *
 * @return The class.
 */
enum stun_class stun_msg_type_get_class(uint16_t type);

/**
 * @brief Find an attribute by type
 *
 * Returns the first attribute of the type that stands ahead of the integrity
 * boundary (see stun_msg::attrs_end): later occurrences, and anything after
 * the boundary, are ignored as the parser ignores them. MESSAGE-INTEGRITY,
 * MESSAGE-INTEGRITY-SHA256 and FINGERPRINT are found where the parser recorded
 * them. Meant for the attributes stun_msg does not carry; the value of a type
 * the codec does not know is not checked, only that it lies inside @p buf.
 *
 * @param buf The datagram @p msg was parsed from.
 * @param len Its length.
 * @param msg Result of stun_parse().
 * @param attr_type Attribute type.
 * @param[out] value Receives a pointer to the value, inside @p buf; written only on
 *              success.
 * @param[out] value_len Receives the length of the value, without padding; written
 *                  only on success.
 *
 * @retval 0 Success.
 * @retval -ENOENT No attribute of the type ahead of the integrity boundary.
 * @retval -EINVAL NULL argument, @p msg does not belong to @p buf, or the
 *                 attribute does not lie inside @p buf.
 */
int stun_find_attr(const uint8_t *buf, size_t len, const struct stun_msg *msg,
		   uint16_t attr_type, const uint8_t **value, uint16_t *value_len);

/**
 * @brief List the comprehension-required attributes nobody understands
 *
 * Lists every attribute of the parsed message that must be understood
 * (comprehension-required, a type of 0x0000 to 0x7FFF) and that neither the
 * codec understands (MAPPED-ADDRESS, USERNAME, MESSAGE-INTEGRITY, ERROR-CODE,
 * UNKNOWN-ATTRIBUTES, REALM, NONCE, MESSAGE-INTEGRITY-SHA256,
 * XOR-MAPPED-ADDRESS, PRIORITY, USE-CANDIDATE) nor the caller, who lists the
 * further types it handles in @p known.
 *
 * If the list is not empty, the message cannot be processed
 * (@rfc{8489,section-6.3}): a request is answered with 420 (Unknown Attribute)
 * and UNKNOWN-ATTRIBUTES, any other message is discarded.
 *
 * @param buf The datagram @p msg was parsed from.
 * @param len Its length.
 * @param msg Result of stun_parse().
 * @param known Further attribute types the caller understands; may be NULL if
 *        @p n_known is 0.
 * @param n_known Number of entries in @p known.
 * @param[out] out Receives the types, in order of appearance, each once; may be
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
 * Recomputes the HMAC-SHA1 of @rfc{8489,section-14.5} and compares it in constant time.
 * The key is the password for short-term credentials.
 *
 * @param buf The datagram @p msg was parsed from.
 * @param len Its length.
 * @param msg Result of stun_parse().
 * @param key Key. For short-term credentials, the password processed with the
 *        OpaqueString profile (@rfc{8265}); ICE passwords are ASCII and need no
 *        processing.
 * @param key_len Its length, not 0.
 *
 * @retval 0 The message is authentic.
 * @retval -ENOENT No MESSAGE-INTEGRITY attribute.
 * @retval -EBADMSG The value does not match.
 * @retval -EINVAL NULL argument, empty key, @p msg does not belong to @p buf, or the
 *                 attribute does not lie inside @p buf.
 * @retval -EIO The cryptographic backend failed.
 */
int stun_verify_integrity(const uint8_t *buf, size_t len, const struct stun_msg *msg,
			  const uint8_t *key, size_t key_len);

/**
 * @brief Verify MESSAGE-INTEGRITY-SHA256
 *
 * Recomputes the HMAC-SHA256 of @rfc{8489,section-14.6} and compares it in constant
 * time. The value has to be the full 32 bytes unless the STUN Usage allows
 * truncation, which the caller states with @p min_len.
 *
 * @param buf The datagram @p msg was parsed from.
 * @param len Its length.
 * @param msg Result of stun_parse().
 * @param key Key. For short-term credentials, the password processed with the
 *        OpaqueString profile (@rfc{8265}); ICE passwords are ASCII and need no
 *        processing.
 * @param key_len Its length, not 0.
 * @param min_len Shortest value to accept: 32, or a multiple of 4 not below 16.
 *
 * @retval 0 The message is authentic.
 * @retval -ENOENT No MESSAGE-INTEGRITY-SHA256 attribute.
 * @retval -EBADMSG The value does not match, or is shorter than @p min_len.
 * @retval -EINVAL NULL argument, empty key, @p min_len out of range, @p msg does not
 *                 belong to @p buf, or the attribute does not lie inside @p buf or has a
 *                 length in @p msg that it cannot have.
 * @retval -EIO The cryptographic backend failed.
 */
int stun_verify_integrity_sha256(const uint8_t *buf, size_t len, const struct stun_msg *msg,
				 const uint8_t *key, size_t key_len, size_t min_len);

/**
 * @brief Verify FINGERPRINT
 *
 * Recomputes the CRC-32 of the message ahead of FINGERPRINT, XORs it with
 * 0x5354554E ("STUN" in ASCII) and compares the result with the value of the
 * attribute (@rfc{8489,section-14.7}). This is no authentication, as there is no
 * key: it tells STUN from other protocols on a shared port and catches a damaged
 * message.
 *
 * @param buf The datagram @p msg was parsed from.
 * @param len Its length.
 * @param msg Result of stun_parse().
 *
 * @retval 0 The fingerprint is correct.
 * @retval -ENOENT No FINGERPRINT attribute.
 * @retval -EBADMSG The CRC-32 does not match: the message is damaged or not STUN.
 * @retval -EINVAL NULL argument, @p msg does not belong to @p buf, or the
 *                 attribute does not lie inside @p buf.
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
 * @param[out] addr Caller-allocated output address with space for a full
 *                  struct net_sockaddr, regardless of the address family.
 *                  Use struct net_sockaddr or struct net_sockaddr_storage.
 *                  Do not pass a smaller family-specific address structure.
 *
 * @retval 0 Success.
 * @retval -ENOENT No such attribute.
 * @retval -EBADMSG The attribute is malformed.
 * @retval -EAFNOSUPPORT A family other than IPv4 and IPv6, or an IPv6 address and the IPv6
 *                       stack is not enabled.
 * @retval -EINVAL NULL argument, @p msg does not belong to @p buf, or the
 *                 attribute does not lie inside @p buf.
 */
int stun_get_xor_address(const uint8_t *buf, size_t len, const struct stun_msg *msg,
			 uint16_t attr_type, struct net_sockaddr *addr);

/**
 * @brief Decode XOR-MAPPED-ADDRESS
 *
 * @param buf The datagram @p msg was parsed from.
 * @param len Its length.
 * @param msg Result of stun_parse().
 * @param[out] addr Caller-allocated output address with space for a full
 *                  struct net_sockaddr, regardless of the address family.
 *                  Use struct net_sockaddr or struct net_sockaddr_storage.
 *                  Do not pass a smaller family-specific address structure.
 *
 * @retval 0 Success.
 * @retval -ENOENT No XOR-MAPPED-ADDRESS attribute.
 * @retval -EAFNOSUPPORT A family other than IPv4 and IPv6, or an IPv6 address and the IPv6
 *                       stack is not enabled.
 * @retval -EINVAL NULL argument, @p msg does not belong to @p buf, or the
 *                 attribute does not lie inside @p buf.
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
 * room (-EMSGSIZE), a failure of the cryptographic backend (-EIO) and an
 * argument stun_builder_init() refused are remembered: every later call
 * returns the same error. A refused argument of any other call (-EINVAL,
 * -EAFNOSUPPORT) or a call out of order (-EPERM) leaves the message as it was
 * and may be followed by further calls.
 */
struct stun_builder {
	/** @cond INTERNAL_HIDDEN */
	uint8_t *buf;
	size_t buf_size;
	size_t msg_len;
	int err;
	uint8_t stage;
	/** @endcond */
};

/**
 * @brief Start a message
 *
 * Writes the message header into @p buf. The function returns nothing: an
 * argument it refuses is remembered in the builder, and every later call on
 * the builder returns it. That is -EINVAL for a NULL @p buf, a NULL
 * @p transaction_id or a @p msg_type with one of its two top bits set, and
 * -EMSGSIZE for a @p buf_size below @ref STUN_HEADER_SIZE.
 *
 * @param[out] builder Builder; if NULL, the function does nothing.
 * @param[out] buf Buffer for the message.
 * @param buf_size Its size, at least @ref STUN_HEADER_SIZE.
 * @param msg_type Message type, 14 bits: the two top bits are zero
 *        (@rfc{8489,section-5}).
 * @param transaction_id Transaction id. For a request it has to come from a
 *        cryptographically secure random generator (@rfc{8489,section-5}).
 */
void stun_builder_init(struct stun_builder *builder, uint8_t *buf, size_t buf_size,
		       uint16_t msg_type, const uint8_t transaction_id[STUN_TXID_SIZE]);

/**
 * @brief Add an attribute by type and value
 *
 * For the attributes of STUN extensions. MESSAGE-INTEGRITY,
 * MESSAGE-INTEGRITY-SHA256 and FINGERPRINT have functions of their own and are
 * refused here.
 *
 * @param[in,out] builder Builder.
 * @param attr_type Attribute type.
 * @param value Value; may be NULL if @p value_len is 0.
 * @param value_len Its length, at most 65535.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p builder is NULL, @p value_len is over 65535, @p value is NULL while
 *         @p value_len is not 0, @p attr_type is one of the closing attributes, or
 *         stun_builder_init() refused its arguments.
 * @retval -EPERM A closing attribute has been added already.
 * @retval -EMSGSIZE The attribute does not fit, or an earlier call ran out of room.
 * @retval -EIO An earlier HMAC failed in the PSA backend.
 */
int stun_add_attr(struct stun_builder *builder, uint16_t attr_type, const uint8_t *value,
		  size_t value_len);

/**
 * @brief Add SOFTWARE
 *
 * @param[in,out] builder Builder.
 * @param sw Description of the software, at most @ref STUN_TEXT_MAX_CHARS
 *        UTF-8 characters.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p builder or @p sw is NULL, @p sw has a UTF-8 sequence with a bad lead or
 *         continuation byte or is longer than @ref STUN_TEXT_MAX_CHARS characters, or
 *         stun_builder_init() refused its arguments.
 * @retval -EPERM A closing attribute has been added already.
 * @retval -EMSGSIZE The attribute does not fit, or an earlier call ran out of room.
 * @retval -EIO An earlier HMAC failed in the PSA backend.
 */
int stun_add_software(struct stun_builder *builder, const char *sw);

/**
 * @brief Add USERNAME
 *
 * @param[in,out] builder Builder.
 * @param username User name, at most @ref STUN_USERNAME_MAX_LEN bytes, already
 *        processed with the OpaqueString profile (@rfc{8265}).
 *
 * @retval 0 Success.
 * @retval -EINVAL @p builder or @p username is NULL, @p username is longer than
 *         @ref STUN_USERNAME_MAX_LEN bytes, or stun_builder_init() refused its arguments.
 * @retval -EPERM A closing attribute has been added already.
 * @retval -EMSGSIZE The attribute does not fit, or an earlier call ran out of room.
 * @retval -EIO An earlier HMAC failed in the PSA backend.
 */
int stun_add_username(struct stun_builder *builder, const char *username);

/**
 * @brief Add an attribute in the XOR-MAPPED-ADDRESS format
 *
 * @param[in,out] builder Builder.
 * @param attr_type Attribute type.
 * @param addr IPv4 or IPv6 address and port.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p builder or @p addr is NULL, @p attr_type is one of the closing attributes, or
 *         stun_builder_init() refused its arguments.
 * @retval -EPERM A closing attribute has been added already.
 * @retval -EAFNOSUPPORT The family of @p addr is neither IPv4 nor IPv6, or it is IPv6 and
 *         @kconfig{CONFIG_NET_IPV6} is not enabled.
 * @retval -EMSGSIZE The attribute does not fit, or an earlier call ran out of room.
 * @retval -EIO An earlier HMAC failed in the PSA backend.
 */
int stun_add_xor_address(struct stun_builder *builder, uint16_t attr_type,
			 const struct net_sockaddr *addr);

/**
 * @brief Add XOR-MAPPED-ADDRESS
 *
 * @param[in,out] builder Builder.
 * @param addr IPv4 or IPv6 address and port.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p builder or @p addr is NULL, or stun_builder_init() refused its arguments.
 * @retval -EPERM A closing attribute has been added already.
 * @retval -EAFNOSUPPORT The family of @p addr is neither IPv4 nor IPv6, or it is IPv6 and
 *         @kconfig{CONFIG_NET_IPV6} is not enabled.
 * @retval -EMSGSIZE The attribute does not fit, or an earlier call ran out of room.
 * @retval -EIO An earlier HMAC failed in the PSA backend.
 */
int stun_add_xor_mapped_address(struct stun_builder *builder, const struct net_sockaddr *addr);

/**
 * @brief Add PRIORITY
 *
 * @param[in,out] builder Builder.
 * @param priority Priority of the candidate (@rfc{8445,section-7.1.1}).
 *
 * @retval 0 Success.
 * @retval -EINVAL @p builder is NULL, or stun_builder_init() refused its arguments.
 * @retval -EPERM A closing attribute has been added already.
 * @retval -EMSGSIZE The attribute does not fit, or an earlier call ran out of room.
 * @retval -EIO An earlier HMAC failed in the PSA backend.
 */
int stun_add_priority(struct stun_builder *builder, uint32_t priority);

/**
 * @brief Add USE-CANDIDATE
 *
 * @param[in,out] builder Builder.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p builder is NULL, or stun_builder_init() refused its arguments.
 * @retval -EPERM A closing attribute has been added already.
 * @retval -EMSGSIZE The attribute does not fit, or an earlier call ran out of room.
 * @retval -EIO An earlier HMAC failed in the PSA backend.
 */
int stun_add_use_candidate(struct stun_builder *builder);

/**
 * @brief Add ERROR-CODE: an error code and its reason phrase
 *
 * @param[in,out] builder Builder.
 * @param code Error code, 300 to 699.
 * @param reason Reason phrase, at most @ref STUN_TEXT_MAX_CHARS UTF-8
 *        characters; NULL for none.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p builder is NULL, @p code is outside 300 to 699, @p reason has a UTF-8 sequence
 *         with a bad lead or continuation byte or is longer than @ref STUN_TEXT_MAX_CHARS
 *         characters, or stun_builder_init() refused its arguments.
 * @retval -EPERM A closing attribute has been added already.
 * @retval -EMSGSIZE The attribute does not fit, or an earlier call ran out of room.
 * @retval -EIO An earlier HMAC failed in the PSA backend.
 */
int stun_add_error_code(struct stun_builder *builder, uint16_t code, const char *reason);

/**
 * @brief Add UNKNOWN-ATTRIBUTES
 *
 * The attribute a 420 error response carries (@rfc{8489,section-6.3.1}); the list is
 * what stun_unknown_attrs() returned for the request.
 *
 * @param[in,out] builder Builder.
 * @param types Attribute types; may be NULL if @p n_types is 0.
 * @param n_types Number of types.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p builder is NULL, @p types is NULL while @p n_types is not 0, @p n_types is
 *         over 32767, or stun_builder_init() refused its arguments.
 * @retval -EPERM A closing attribute has been added already.
 * @retval -EMSGSIZE The attribute does not fit, or an earlier call ran out of room.
 * @retval -EIO An earlier HMAC failed in the PSA backend.
 */
int stun_add_unknown_attributes(struct stun_builder *builder, const uint16_t *types,
				size_t n_types);

/**
 * @brief Add ICE-CONTROLLING
 *
 * @param[in,out] builder Builder.
 * @param tiebreaker Tie-breaker of the agent (@rfc{8445,section-7.1.3}).
 *
 * @retval 0 Success.
 * @retval -EINVAL @p builder is NULL, or stun_builder_init() refused its arguments.
 * @retval -EPERM A closing attribute has been added already.
 * @retval -EMSGSIZE The attribute does not fit, or an earlier call ran out of room.
 * @retval -EIO An earlier HMAC failed in the PSA backend.
 */
int stun_add_ice_controlling(struct stun_builder *builder, uint64_t tiebreaker);

/**
 * @brief Add ICE-CONTROLLED
 *
 * @param[in,out] builder Builder.
 * @param tiebreaker Tie-breaker of the agent (@rfc{8445,section-7.1.3}).
 *
 * @retval 0 Success.
 * @retval -EINVAL @p builder is NULL, or stun_builder_init() refused its arguments.
 * @retval -EPERM A closing attribute has been added already.
 * @retval -EMSGSIZE The attribute does not fit, or an earlier call ran out of room.
 * @retval -EIO An earlier HMAC failed in the PSA backend.
 */
int stun_add_ice_controlled(struct stun_builder *builder, uint64_t tiebreaker);

/**
 * @brief Add MESSAGE-INTEGRITY
 *
 * Signs what the message holds so far with HMAC-SHA1 (@rfc{8489,section-14.5}). After
 * it only MESSAGE-INTEGRITY-SHA256 and FINGERPRINT can be added.
 *
 * @param[in,out] builder Builder.
 * @param key Key. For short-term credentials, the password processed with the
 *        OpaqueString profile (@rfc{8265}); ICE passwords are ASCII and need no
 *        processing.
 * @param key_len Its length, not 0.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p builder or @p key is NULL, @p key_len is 0, or stun_builder_init() refused its
 *         arguments.
 * @retval -EPERM MESSAGE-INTEGRITY or a later closing attribute has been added already.
 * @retval -EMSGSIZE The attribute does not fit, or an earlier call ran out of room.
 * @retval -EIO The PSA backend failed to compute the HMAC, now or in an earlier call.
 */
int stun_add_integrity(struct stun_builder *builder, const uint8_t *key, size_t key_len);

/**
 * @brief Add MESSAGE-INTEGRITY-SHA256
 *
 * Signs what the message holds so far with HMAC-SHA256, not truncated
 * (@rfc{8489,section-14.6}). After it only FINGERPRINT can be added.
 * @rfc{8489,section-9.1.2}: a request or an indication carries both integrity
 * attributes unless the two agents know by other means which one they share, as
 * ICE agents do.
 *
 * @param[in,out] builder Builder.
 * @param key Key. For short-term credentials, the password processed with the
 *        OpaqueString profile (@rfc{8265}); ICE passwords are ASCII and need no
 *        processing.
 * @param key_len Its length, not 0.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p builder or @p key is NULL, @p key_len is 0, or stun_builder_init() refused its
 *         arguments.
 * @retval -EPERM MESSAGE-INTEGRITY-SHA256 or FINGERPRINT has been added already.
 * @retval -EMSGSIZE The attribute does not fit, or an earlier call ran out of room.
 * @retval -EIO The PSA backend failed to compute the HMAC, now or in an earlier call.
 */
int stun_add_integrity_sha256(struct stun_builder *builder, const uint8_t *key, size_t key_len);

/**
 * @brief Add FINGERPRINT
 *
 * The last attribute of a message (@rfc{8489,section-14.7}): nothing can be added after
 * it.
 *
 * @param[in,out] builder Builder.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p builder is NULL, or stun_builder_init() refused its arguments.
 * @retval -EPERM FINGERPRINT has been added already.
 * @retval -EMSGSIZE The attribute does not fit, or an earlier call ran out of room.
 * @retval -EIO An earlier HMAC failed in the PSA backend.
 */
int stun_add_fingerprint(struct stun_builder *builder);

/**
 * @brief Get the length of the message built so far
 *
 * @param builder Builder.
 *
 * @return The length of the message in bytes, if no call has failed.
 * @retval -EINVAL @p builder is NULL, or stun_builder_init() refused its arguments.
 * @retval -EMSGSIZE An earlier call ran out of room.
 * @retval -EIO An earlier HMAC failed in the PSA backend.
 */
int stun_builder_len(const struct stun_builder *builder);

/**
 * @brief Close a message with MESSAGE-INTEGRITY and FINGERPRINT
 *
 * Signs the message with HMAC-SHA1 over @p key, adds FINGERPRINT and returns the
 * length of the finished message, ready to be sent; nothing can be added to it
 * afterwards. The same as stun_add_integrity() followed by stun_finish_plain().
 * This is what an ICE agent ends its connectivity checks and their responses
 * with: a check must be signed with the short-term credential
 * (@rfc{8445,section-7.2.2}) and carry FINGERPRINT (@rfc{8445,section-7.2.4}),
 * and so must the response (@rfc{8445,section-7.3}).
 *
 * The key is required. A message that is not to be signed is closed with
 * stun_finish_plain() instead. The two are kept apart, rather than one function
 * taking a NULL key for "no signature", so that a missing key fails here with
 * -EINVAL instead of quietly producing a message without MESSAGE-INTEGRITY,
 * which a peer that expects one would discard.
 *
 * @param[in,out] builder Builder.
 * @param key Key. For short-term credentials, the password processed with the
 *        OpaqueString profile (@rfc{8265}); ICE passwords are ASCII and need no
 *        processing.
 * @param key_len Its length, not 0.
 *
 * @return The length of the message in bytes.
 * @retval -EINVAL @p builder or @p key is NULL, @p key_len is 0, or stun_builder_init() refused its
 *         arguments.
 * @retval -EPERM MESSAGE-INTEGRITY or a later closing attribute has been added already.
 * @retval -EMSGSIZE The closing attributes do not fit, or an earlier call ran out of room.
 * @retval -EIO The PSA backend failed to compute the HMAC, now or in an earlier call.
 */
int stun_finish(struct stun_builder *builder, const uint8_t *key, size_t key_len);

/**
 * @brief Close a message with FINGERPRINT alone
 *
 * Adds FINGERPRINT and returns the length of the finished message, ready to be
 * sent; nothing can be added to it afterwards. The same as
 * stun_add_fingerprint() followed by stun_builder_len().
 *
 * For a message that carries no integrity attribute, such as a Binding request
 * to a public STUN server, which has no credential to check. It also closes a
 * message the caller has signed itself, for example with both
 * stun_add_integrity() and stun_add_integrity_sha256() as
 * @rfc{8489,section-9.1.2} asks of a request or an indication when the agents do
 * not know which algorithm they share; stun_finish() adds MESSAGE-INTEGRITY
 * alone.
 *
 * Unlike stun_finish() it takes no key, so choosing it is choosing not to sign
 * here; see stun_finish() for why the two are separate.
 *
 * @param[in,out] builder Builder.
 *
 * @return The length of the message in bytes.
 * @retval -EINVAL @p builder is NULL, or stun_builder_init() refused its arguments.
 * @retval -EPERM FINGERPRINT has been added already.
 * @retval -EMSGSIZE FINGERPRINT does not fit, or an earlier call ran out of room.
 * @retval -EIO An earlier HMAC failed in the PSA backend.
 */
int stun_finish_plain(struct stun_builder *builder);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_NET_STUN_H_ */
