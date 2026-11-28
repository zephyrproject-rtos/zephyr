/*
 * Copyright (c) 2026 StructCore Embedded
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_SNMP_SNMP_USM_H_
#define ZEPHYR_INCLUDE_NET_SNMP_SNMP_USM_H_

#include <zephyr/kernel.h>
#include <zephyr/net/snmp/snmp_ber.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup snmp_usm User-Based Security Model (USM)
 * @ingroup snmp
 * @{
 * @brief RFC 3414 / RFC 7860 authentication, encryption, and key localization.
 */

/*
 * Maximum USM users per instance. Normally provided by Kconfig (SNMP_V3_MAX_USERS,
 * depends on SNMP_VERSION_3); the fallback keeps the header usable in host builds
 * (tests/unit) that do not run the Kconfig preprocessing step.
 */
#ifndef CONFIG_SNMP_V3_MAX_USERS
#define CONFIG_SNMP_V3_MAX_USERS 4
#endif

/* Authentication Protocols (RFC 3414, RFC 7860) */
#define SNMP_USM_AUTH_NONE   0
#define SNMP_USM_AUTH_SHA1   1
#define SNMP_USM_AUTH_MD5    2
#define SNMP_USM_AUTH_SHA256 3

/* Privacy Protocols (RFC 3414, RFC 3826) */
#define SNMP_USM_PRIV_NONE 0
#define SNMP_USM_PRIV_AES  1
#define SNMP_USM_PRIV_DES  2

/* Forward declaration of agent instance */
struct snmp_agent;

struct snmp_usm_user {
	char user_name[32];
	uint8_t auth_protocol; /* SNMP_USM_AUTH_* */
	char auth_pass[64];
	uint8_t priv_protocol; /* SNMP_USM_PRIV_* */
	char priv_pass[64];

	/* Localized keys (derived at init) */
	uint8_t auth_key[32]; /* SHA1=20, MD5=16, SHA256=32 */
	uint16_t auth_key_len;
	uint8_t priv_key[20]; /* AES=16, DES=16 */
	uint16_t priv_key_len;
	bool in_use;
};

/* ---- Instance-based APIs ---- */

/**
 * @brief Initialize the USM engine of an agent instance (engine ID, engineBoots,
 *        and localization of any pre-loaded user keys).
 * @param inst Target agent instance.
 * @return 0 on success, -EINVAL if @p inst is NULL.
 */
int snmp_usm_init(struct snmp_agent *inst);

/**
 * @brief Add or update a USM user in the agent's local keys table.
 * @param inst Target agent instance.
 * @param user User record; passwords are localized against the instance engine ID.
 * @return 0 on success, -EINVAL on bad arguments, -ENOMEM if the table is full.
 */
int snmp_usm_add_user(struct snmp_agent *inst, const struct snmp_usm_user *user);

/**
 * @brief Copy the agent's snmpEngineID.
 * @param inst Target agent instance.
 * @param engine_id Output buffer.
 * @param len In: capacity, out: written length.
 * @return 0 on success, -EINVAL on bad arguments, -ENOBUFS if the buffer is too small.
 */
int snmp_usm_get_engine_id(struct snmp_agent *inst, uint8_t *engine_id, uint8_t *len);

/**
 * @brief Set snmpEngineBoots (RFC 3414 section 5.2.5). Persisted per instance when a
 *        settings prefix is configured.
 * @param inst Target agent instance.
 * @param boots New value, not above 2147483647.
 * @return 0 on success, -EINVAL on bad arguments.
 */
int snmp_usm_set_engine_boots(struct snmp_agent *inst, uint32_t boots);

/**
 * @brief Read snmpEngineBoots of an agent instance. Safe to call from any context.
 * @param inst Target agent instance.
 * @return Current engineBoots, 0 if @p inst is NULL.
 */
uint32_t snmp_usm_get_engine_boots(struct snmp_agent *inst);

/**
 * @brief Read snmpEngineTime of an agent instance (RFC 3414 §2.2). Safe to call from any context.
 * @param inst Target agent instance.
 * @return Current engineTime in seconds, 0 if @p inst is NULL.
 */
uint32_t snmp_usm_get_engine_time(struct snmp_agent *inst);

/**
 * @brief Set snmpEngineTime epoch base for an agent instance (RFC 3414 §2.2).
 * @param inst Target agent instance.
 * @param time New engineTime value in seconds.
 * @return 0 on success, -EINVAL if @p inst is NULL.
 */
int snmp_usm_set_engine_time(struct snmp_agent *inst, uint32_t time);

/**
 * @brief Register RFC 3411 (snmpFrameworkMIB), RFC 3412 (snmpMPD) and RFC 3414
 *        (usmMIB stats) managed objects in the agent's MIB tree.
 * @param inst Target agent instance.
 * @return 0 on success, negative errno on failure.
 */
int snmp_usm_register_mibs(struct snmp_agent *inst);

/** @} */

/*
 * snmp_usm_verify_incoming() and snmp_usm_build_outgoing() are module-internal:
 * they are called across translation units (snmp_core.c, snmp_trap.c) and are
 * declared in src/snmp_internal.h, not here.
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_SNMP_SNMP_USM_H_ */
