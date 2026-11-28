/*
 * Copyright (c) 2026 StructCore Embedded
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_SNMP_SNMP_BER_H_
#define ZEPHYR_INCLUDE_NET_SNMP_SNMP_BER_H_

#include <zephyr/kernel.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup snmp_ber ASN.1 BER Codec
 * @ingroup snmp
 * @{
 * @brief Basic Encoding Rules (BER) tag/length/value encoders and decoders.
 */

/* --- ASN.1 Primitive & Application Tags --- */
#define ASN1_TAG_INTEGER      0x02
#define ASN1_TAG_OCTET_STRING 0x04
#define ASN1_TAG_NULL         0x05
#define ASN1_TAG_OID          0x06
#define ASN1_TAG_SEQUENCE     0x30

#define SNMP_TAG_IPADDRESS  0x40
#define SNMP_TAG_COUNTER32  0x41
#define SNMP_TAG_GAUGE32    0x42
#define SNMP_TAG_TIMETICKS  0x43
#define SNMP_TAG_OPAQUE     0x44
#define SNMP_TAG_COUNTER64  0x46
#define SNMP_TAG_UNSIGNED32 0x47

/* --- SNMP Exception Tags --- */
#define SNMP_TAG_EXCEPT_NO_SUCH_OBJ  0x80
#define SNMP_TAG_EXCEPT_NO_SUCH_INST 0x81
#define SNMP_TAG_EXCEPT_END_MIB_VIEW 0x82

/* --- SNMP PDU Tags --- */
#define SNMP_PDU_GET_REQ      0xA0
#define SNMP_PDU_GET_NEXT_REQ 0xA1
#define SNMP_PDU_GET_RESP     0xA2
#define SNMP_PDU_RESPONSE     SNMP_PDU_GET_RESP
#define SNMP_PDU_SET_REQ      0xA3
#define SNMP_PDU_TRAP_V1      0xA4
#define SNMP_PDU_GET_BULK_REQ 0xA5
#define SNMP_PDU_INFORM_REQ   0xA6
#define SNMP_PDU_TRAP_V2      0xA7
#define SNMP_PDU_REPORT       0xA8

/* --- Generic Trap Types (v1) --- */
#define SNMP_TRAP_COLDSTART       0
#define SNMP_TRAP_WARMSTART       1
#define SNMP_TRAP_LINKDOWN        2
#define SNMP_TRAP_LINKUP          3
#define SNMP_TRAP_AUTHFAIL        4
#define SNMP_TRAP_EGPNEIGHBORLOSS 5
#define SNMP_TRAP_ENTERPRISESPEC  6

/* --- Data Types & Limits --- */
struct snmp_oid {
	uint8_t len;
	uint32_t ids[CONFIG_SNMP_MAX_OID_LEN];
};

union snmp_val {
	int32_t int_val;
	uint32_t uint_val;
	uint64_t uint64_val;
	struct {
		uint8_t data[128];
		uint16_t len;
	} octet_str;
	struct snmp_oid oid;
	uint8_t ip_addr[4];
};

struct snmp_varbind {
	struct snmp_oid oid;
	uint8_t type;
	union snmp_val val;
};

/* Opaque PDU Structure */
struct snmp_pdu {
	int32_t version;
	uint8_t community[64];
	uint16_t community_len;
	uint8_t type;
	int32_t request_id;
	int32_t error_status;
	int32_t error_index;
	int32_t non_repeaters;
	int32_t max_repetitions;

	/* Trap v1 fields */
	struct snmp_oid enterprise;
	uint8_t agent_addr[4];
	int32_t generic_trap;
	int32_t specific_trap;
	uint32_t timestamp;

	/* Variable Bindings */
	uint8_t varbind_cnt;
	struct snmp_varbind varbinds[CONFIG_SNMP_MAX_VARBINDS];
};

/* --- SNMPv3 Message Structures (RFC 3412) --- */

#define SNMP_V3_ENGINE_ID_MAX_LEN 32
#define SNMP_V3_AUTH_PARAMS_LEN   32 /* HMAC-SHA1 (12), HMAC-MD5 (12), SHA-256 (24) */
#define SNMP_V3_PRIV_PARAMS_LEN   16 /* AES-128-CFB IV */

struct snmp_v3_global_data {
	int32_t msg_id;
	int32_t msg_max_size;
	uint8_t msg_flags;          /* bit0=auth, bit1=priv, bit2=reportable */
	int32_t msg_security_model; /* 3 = USM */
};

struct snmp_v3_usm_params {
	uint8_t engine_id[SNMP_V3_ENGINE_ID_MAX_LEN];
	uint16_t engine_id_len;
	int32_t engine_boots;
	int32_t engine_time;
	uint8_t user_name[32];
	uint16_t user_name_len;
	uint8_t auth_params[SNMP_V3_AUTH_PARAMS_LEN];
	uint16_t auth_params_len;
	/**
	 * @brief Absolute offset of the authentication parameter value inside the
	 *        received datagram.
	 *
	 * USM must compute the HMAC over the message *as received* with this field
	 * zeroed (RFC 3414 section 6.3.1), not over a re-encoded copy: a peer that
	 * uses a different but valid BER length encoding would otherwise fail
	 * authentication. Set by snmp_ber_decode_v3_msg(); 0 means "not decoded
	 * from a datagram" (outgoing message).
	 */
	size_t auth_params_raw_off;
	uint8_t priv_params[SNMP_V3_PRIV_PARAMS_LEN];
	uint16_t priv_params_len;
};

struct snmp_v3_scoped_pdu {
	uint8_t context_engine_id[SNMP_V3_ENGINE_ID_MAX_LEN];
	uint16_t context_engine_id_len;
	uint8_t context_name[32];
	uint16_t context_name_len;
	struct snmp_pdu pdu;      /* embedded v1/v2c-style PDU */
	size_t encrypted_raw_off; /* offset in raw packet (priv mode) */
	size_t encrypted_raw_len; /* length of encrypted data (priv mode) */
	uint8_t priv_ciphertext[CONFIG_SNMP_MAX_PDU_SIZE];
	size_t priv_ciphertext_len;
};

struct snmp_v3_msg {
	struct snmp_v3_global_data global;
	struct snmp_v3_usm_params usm;
	struct snmp_v3_scoped_pdu scoped;
	/**
	 * @brief Security level actually verified for this message.
	 *
	 * SNMP_SEC_LEVEL_NOAUTH_NOPRIV / AUTH_NOPRIV / AUTH_PRIV, set by
	 * snmp_usm_verify_incoming() from the *configured user* and the checks that
	 * actually succeeded. It is deliberately not derived from msgFlags: a
	 * noAuthNoPriv user can otherwise set the auth/priv bits and reach VACM
	 * rules that require authentication.
	 */
	uint8_t sec_level;
};

/* --- Function Declarations --- */

/* Primitive decoders */
int snmp_ber_decode_header(const uint8_t *buf, size_t buf_len, size_t *off, uint8_t *tag,
			   size_t *len);
int snmp_ber_decode_int(const uint8_t *buf, size_t buf_len, size_t *off, int32_t *val);
int snmp_ber_decode_uint(const uint8_t *buf, size_t buf_len, size_t *off, uint32_t *val);
int snmp_ber_decode_uint64(const uint8_t *buf, size_t buf_len, size_t *off, uint64_t *val);
int snmp_ber_decode_octet_string(const uint8_t *buf, size_t buf_len, size_t *off, uint8_t *out,
				 uint16_t max_out, uint16_t *out_len);
int snmp_ber_decode_oid(const uint8_t *buf, size_t buf_len, size_t *off, struct snmp_oid *oid);

/* Primitive encoders */
int snmp_ber_encode_header(uint8_t *buf, size_t buf_len, size_t *off, uint8_t tag, size_t len);
int snmp_ber_encode_int(uint8_t *buf, size_t buf_len, size_t *off, uint8_t tag, int32_t val);
int snmp_ber_encode_uint(uint8_t *buf, size_t buf_len, size_t *off, uint8_t tag, uint32_t val);
int snmp_ber_encode_uint64(uint8_t *buf, size_t buf_len, size_t *off, uint8_t tag, uint64_t val);
int snmp_ber_encode_octet_string(uint8_t *buf, size_t buf_len, size_t *off, uint8_t tag,
				 const uint8_t *str, uint16_t str_len);
int snmp_ber_encode_oid(uint8_t *buf, size_t buf_len, size_t *off, const struct snmp_oid *oid);
int snmp_ber_encode_null(uint8_t *buf, size_t buf_len, size_t *off, uint8_t tag);

/* PDU level APIs (v1/v2c) */
int snmp_ber_decode_pdu(const uint8_t *buf, size_t buf_len, struct snmp_pdu *pdu);
int snmp_ber_decode_pdu_body(const uint8_t *buf, size_t buf_len, size_t *off, struct snmp_pdu *pdu);
int snmp_ber_encode_pdu(uint8_t *buf, size_t buf_len, size_t *out_len, const struct snmp_pdu *pdu);
int snmp_ber_encode_pdu_body(uint8_t *buf, size_t buf_len, size_t *out_len,
			     const struct snmp_pdu *pdu);

/* PDU level APIs (v3) */
int snmp_ber_decode_v3_msg(const uint8_t *buf, size_t buf_len, struct snmp_v3_msg *msg);
int snmp_ber_encode_v3_msg(uint8_t *buf, size_t buf_len, size_t *out_len,
			   const struct snmp_v3_msg *msg);

/* Utility helpers */
bool snmp_oid_equal(const struct snmp_oid *o1, const struct snmp_oid *o2);
int snmp_oid_compare(const struct snmp_oid *o1, const struct snmp_oid *o2);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_SNMP_SNMP_BER_H_ */
