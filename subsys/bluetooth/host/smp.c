/**
 * @file smp.c
 * Security Manager Protocol implementation
 */

/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/debug/stack.h>
#include <zephyr/kernel.h>
#include <zephyr/net/buf.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/util.h>


#include "common/bt_str.h"

#include "crypto/bt_crypto.h"

#include "hci_core.h"
#include "ecc.h"
#include "keys.h"
#include "conn_internal.h"
#include "l2cap_internal.h"
#include "classic/l2cap_br_interface.h"
#include "smp.h"

#define LOG_LEVEL CONFIG_BT_SMP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_smp);

#define SMP_TIMEOUT K_SECONDS(30)

#if defined(CONFIG_BT_SIGNING)
#define SIGN_DIST BT_SMP_DIST_SIGN
#else
#define SIGN_DIST 0
#endif

#if defined(CONFIG_BT_PRIVACY)
#define ID_DIST BT_SMP_DIST_ID_KEY
#else
#define ID_DIST 0
#endif

#if defined(CONFIG_BT_CLASSIC)
#define LINK_DIST BT_SMP_DIST_LINK_KEY
#else
#define LINK_DIST 0
#endif

#define RECV_KEYS (BT_SMP_DIST_ENC_KEY | BT_SMP_DIST_ID_KEY | SIGN_DIST |\
		   LINK_DIST)
#define SEND_KEYS (BT_SMP_DIST_ENC_KEY | ID_DIST | SIGN_DIST | LINK_DIST)

#define RECV_KEYS_SC (RECV_KEYS & ~(BT_SMP_DIST_ENC_KEY))
#define SEND_KEYS_SC (SEND_KEYS & ~(BT_SMP_DIST_ENC_KEY))

#define BR_RECV_KEYS_SC (RECV_KEYS & ~(LINK_DIST))
#define BR_SEND_KEYS_SC (SEND_KEYS & ~(LINK_DIST))

#define BT_SMP_AUTH_MASK	0x07

#if defined(CONFIG_BT_BONDABLE)
#define BT_SMP_AUTH_BONDING_FLAGS BT_SMP_AUTH_BONDING
#else
#define BT_SMP_AUTH_BONDING_FLAGS 0
#endif /* CONFIG_BT_BONDABLE */

#if defined(CONFIG_BT_CLASSIC)

#define BT_SMP_AUTH_MASK_SC	0x2f
#if defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)
#define BT_SMP_AUTH_DEFAULT (BT_SMP_AUTH_BONDING_FLAGS | BT_SMP_AUTH_CT2)
#else
#define BT_SMP_AUTH_DEFAULT (BT_SMP_AUTH_BONDING_FLAGS | BT_SMP_AUTH_CT2 |\
			     BT_SMP_AUTH_SC)
#endif /* CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY */

#else

#define BT_SMP_AUTH_MASK_SC	0x0f
#if defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)
#define BT_SMP_AUTH_DEFAULT (BT_SMP_AUTH_BONDING_FLAGS)
#else
#define BT_SMP_AUTH_DEFAULT (BT_SMP_AUTH_BONDING_FLAGS | BT_SMP_AUTH_SC)
#endif /* CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY */

#endif /* CONFIG_BT_CLASSIC */

enum pairing_method {
	JUST_WORKS,		/* JustWorks pairing */
	PASSKEY_INPUT,		/* Passkey Entry input */
	PASSKEY_DISPLAY,	/* Passkey Entry display */
	PASSKEY_CONFIRM,	/* Passkey confirm */
	PASSKEY_ROLE,		/* Passkey Entry depends on role */
	LE_SC_OOB,		/* LESC Out of Band */
	LEGACY_OOB,		/* Legacy Out of Band */
};

enum {
	SMP_FLAG_CFM_DELAYED,	/* if confirm should be send when TK is valid */
	SMP_FLAG_ENC_PENDING,	/* if waiting for an encryption change event */
	SMP_FLAG_KEYS_DISTR,	/* if keys distribution phase is in progress */
	SMP_FLAG_PAIRING,	/* if pairing is in progress */
	SMP_FLAG_TIMEOUT,	/* if SMP timeout occurred */
	SMP_FLAG_SC,		/* if LE Secure Connections is used */
	SMP_FLAG_PKEY_SEND,	/* if should send Public Key when available */
	SMP_FLAG_DHKEY_PENDING,	/* if waiting for local DHKey */
	SMP_FLAG_DHKEY_GEN,     /* if generating DHKey */
	SMP_FLAG_DHKEY_SEND,	/* if should generate and send DHKey Check */
	SMP_FLAG_USER,		/* if waiting for user input */
	SMP_FLAG_DISPLAY,       /* if display_passkey() callback was called */
	SMP_FLAG_OOB_PENDING,	/* if waiting for OOB data */
	SMP_FLAG_BOND,		/* if bonding */
	SMP_FLAG_SC_DEBUG_KEY,	/* if Secure Connection are using debug key */
	SMP_FLAG_SEC_REQ,	/* if Security Request was sent/received */
	SMP_FLAG_DHCHECK_WAIT,	/* if waiting for remote DHCheck (as periph) */
	SMP_FLAG_DERIVE_LK,	/* if Link Key should be derived */
	SMP_FLAG_BR_CONNECTED,	/* if BR/EDR channel is connected */
	SMP_FLAG_BR_PAIR,	/* if should start BR/EDR pairing */
	SMP_FLAG_CT2,		/* if should use H7 for keys derivation */

	/* Total number of flags - must be at the end */
	SMP_NUM_FLAGS,
};

/* SMP channel specific context */
struct bt_smp {
	/* Commands that remote is allowed to send */
	ATOMIC_DEFINE(allowed_cmds, BT_SMP_NUM_CMDS);

	/* Flags for SMP state machine */
	ATOMIC_DEFINE(flags, SMP_NUM_FLAGS);

	/* Type of method used for pairing */
	uint8_t				method;

	/* Pairing Request PDU */
	uint8_t				preq[7];

	/* Pairing Response PDU */
	uint8_t				prsp[7];

	/* Pairing Confirm PDU */
	uint8_t				pcnf[16];

	/* Local random number */
	uint8_t				prnd[16];

	/* Remote random number */
	uint8_t				rrnd[16];

	/* Temporary key */
	uint8_t				tk[16];

	/* Remote Public Key for LE SC */
	uint8_t				pkey[BT_PUB_KEY_LEN];

	/* DHKey */
	uint8_t				dhkey[BT_DH_KEY_LEN];

	/* Remote DHKey check */
	uint8_t				e[16];

	/* MacKey */
	uint8_t				mackey[16];

	/* LE SC passkey */
	uint32_t				passkey;

	/* LE SC passkey round */
	uint8_t				passkey_round;

	/* LE SC local OOB data */
	const struct bt_le_oob_sc_data	*oobd_local;

	/* LE SC remote OOB data */
	const struct bt_le_oob_sc_data	*oobd_remote;

	/* Local key distribution */
	uint8_t				local_dist;

	/* Remote key distribution */
	uint8_t				remote_dist;

	/* The channel this context is associated with.
	 * This marks the beginning of the part of the structure that will not
	 * be memset to zero in init.
	 */
	struct bt_l2cap_le_chan		chan;

	/* Delayed work for timeout handling */
	struct k_work_delayable		work;

	/* Used Bluetooth authentication callbacks. */
	atomic_ptr_t			auth_cb;

	/* Bondable flag */
	atomic_t			bondable;
};

static unsigned int fixed_passkey = BT_PASSKEY_INVALID;

#define DISPLAY_FIXED(smp) (IS_ENABLED(CONFIG_BT_FIXED_PASSKEY) && \
			    fixed_passkey != BT_PASSKEY_INVALID && \
			    (smp)->method == PASSKEY_DISPLAY)

#if !defined(CONFIG_BT_SMP_SC_PAIR_ONLY)
/* based on table 2.8 Core Spec 2.3.5.1 Vol. 3 Part H */
static const uint8_t gen_method_legacy[5 /* remote */][5 /* local */] = {
	{ JUST_WORKS, JUST_WORKS, PASSKEY_INPUT, JUST_WORKS, PASSKEY_INPUT },
	{ JUST_WORKS, JUST_WORKS, PASSKEY_INPUT, JUST_WORKS, PASSKEY_INPUT },
	{ PASSKEY_DISPLAY, PASSKEY_DISPLAY, PASSKEY_INPUT, JUST_WORKS,
	  PASSKEY_DISPLAY },
	{ JUST_WORKS, JUST_WORKS, JUST_WORKS, JUST_WORKS, JUST_WORKS },
	{ PASSKEY_DISPLAY, PASSKEY_DISPLAY, PASSKEY_INPUT, JUST_WORKS,
	  PASSKEY_ROLE },
};
#endif /* CONFIG_BT_SMP_SC_PAIR_ONLY */

#if !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)
/* based on table 2.8 Core Spec 2.3.5.1 Vol. 3 Part H */
static const uint8_t gen_method_sc[5 /* remote */][5 /* local */] = {
	{ JUST_WORKS, JUST_WORKS, PASSKEY_INPUT, JUST_WORKS, PASSKEY_INPUT },
	{ JUST_WORKS, PASSKEY_CONFIRM, PASSKEY_INPUT, JUST_WORKS,
	  PASSKEY_CONFIRM },
	{ PASSKEY_DISPLAY, PASSKEY_DISPLAY, PASSKEY_INPUT, JUST_WORKS,
	  PASSKEY_DISPLAY },
	{ JUST_WORKS, JUST_WORKS, JUST_WORKS, JUST_WORKS, JUST_WORKS },
	{ PASSKEY_DISPLAY, PASSKEY_CONFIRM, PASSKEY_INPUT, JUST_WORKS,
	  PASSKEY_CONFIRM },
};
#endif /* !CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY */

#if defined(CONFIG_BT_CLASSIC)
/* SMP over BR/EDR channel specific context */
struct bt_smp_br {
	/* Commands that remote is allowed to send */
	ATOMIC_DEFINE(allowed_cmds, BT_SMP_NUM_CMDS);

	/* Flags for SMP state machine */
	ATOMIC_DEFINE(flags, SMP_NUM_FLAGS);

	/* Local key distribution */
	uint8_t			local_dist;

	/* Remote key distribution */
	uint8_t			remote_dist;

	/* Encryption Key Size used for connection */
	uint8_t 			enc_key_size;

	/* The channel this context is associated with.
	 * This marks the beginning of the part of the structure that will not
	 * be memset to zero in init.
	 */
	struct bt_l2cap_br_chan	chan;

	/* Delayed work for timeout handling */
	struct k_work_delayable	work;
};

static struct bt_smp_br bt_smp_br_pool[CONFIG_BT_MAX_CONN];
#endif /* CONFIG_BT_CLASSIC */

static struct bt_smp bt_smp_pool[CONFIG_BT_MAX_CONN];
static bool bondable = IS_ENABLED(CONFIG_BT_BONDABLE);
static bool sc_oobd_present;
static bool legacy_oobd_present;
static bool sc_supported;
static const uint8_t *sc_public_key;
static K_SEM_DEFINE(sc_local_pkey_ready, 0, 1);

/* Pointer to internal data is used to mark that callbacks of given SMP channel are not initialized.
 * Value of NULL represents no authentication capabilities and cannot be used for that purpose.
 */
#define BT_SMP_AUTH_CB_UNINITIALIZED	((atomic_ptr_val_t)bt_smp_pool)

/* Value used to mark that per-connection bondable flag is not initialized.
 * Value false/true represent if flag is cleared or set and cannot be used for that purpose.
 */
#define BT_SMP_BONDABLE_UNINITIALIZED	((atomic_val_t)-1)

static bool le_sc_supported(void)
{
	/*
	 * If controller based ECC is to be used it must support
	 * "LE Read Local P-256 Public Key" and "LE Generate DH Key" commands.
	 * Otherwise LE SC are not supported.
	 */
	if (IS_ENABLED(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)) {
		return false;
	}

	return BT_CMD_TEST(bt_dev.supported_commands, 34, 1) &&
	       BT_CMD_TEST(bt_dev.supported_commands, 34, 2);
}

static const struct bt_conn_auth_cb *latch_auth_cb(struct bt_smp *smp)
{
	(void)atomic_ptr_cas(&smp->auth_cb, BT_SMP_AUTH_CB_UNINITIALIZED,
			     (atomic_ptr_val_t)bt_auth);

	return atomic_ptr_get(&smp->auth_cb);
}

static bool latch_bondable(struct bt_smp *smp)
{
	(void)atomic_cas(&smp->bondable, BT_SMP_BONDABLE_UNINITIALIZED, (atomic_val_t)bondable);

	return atomic_get(&smp->bondable);
}

static uint8_t get_io_capa(struct bt_smp *smp)
{
	const struct bt_conn_auth_cb *smp_auth_cb = latch_auth_cb(smp);

	if (!smp_auth_cb) {
		goto no_callbacks;
	}

	/* Passkey Confirmation is valid only for LE SC */
	if (smp_auth_cb->passkey_display && smp_auth_cb->passkey_entry &&
	    (smp_auth_cb->passkey_confirm || !sc_supported)) {
		return BT_SMP_IO_KEYBOARD_DISPLAY;
	}

	/* DisplayYesNo is useful only for LE SC */
	if (sc_supported && smp_auth_cb->passkey_display &&
	    smp_auth_cb->passkey_confirm) {
		return BT_SMP_IO_DISPLAY_YESNO;
	}

	if (smp_auth_cb->passkey_entry) {
		if (IS_ENABLED(CONFIG_BT_FIXED_PASSKEY) &&
		    fixed_passkey != BT_PASSKEY_INVALID) {
			return BT_SMP_IO_KEYBOARD_DISPLAY;
		} else {
			return BT_SMP_IO_KEYBOARD_ONLY;
		}
	}

	if (smp_auth_cb->passkey_display) {
		return BT_SMP_IO_DISPLAY_ONLY;
	}

no_callbacks:
	if (IS_ENABLED(CONFIG_BT_FIXED_PASSKEY) &&
	    fixed_passkey != BT_PASSKEY_INVALID) {
		return BT_SMP_IO_DISPLAY_ONLY;
	} else {
		return BT_SMP_IO_NO_INPUT_OUTPUT;
	}
}

#if !defined(CONFIG_BT_SMP_SC_PAIR_ONLY)
static uint8_t legacy_get_pair_method(struct bt_smp *smp, uint8_t remote_io);
#endif

static bool smp_keys_check(struct bt_conn *conn)
{
	if (atomic_test_bit(conn->flags, BT_CONN_FORCE_PAIR)) {
		return false;
	}

	if (!conn->le.keys) {
		conn->le.keys = bt_keys_find(BT_KEYS_LTK_P256,
						     conn->id, &conn->le.dst);
		if (!conn->le.keys) {
			conn->le.keys = bt_keys_find(BT_KEYS_LTK,
						     conn->id,
						     &conn->le.dst);
		}
	}

	if (!conn->le.keys ||
	    !(conn->le.keys->keys & (BT_KEYS_LTK | BT_KEYS_LTK_P256))) {
		return false;
	}

	if (conn->required_sec_level >= BT_SECURITY_L3 &&
	    !(conn->le.keys->flags & BT_KEYS_AUTHENTICATED)) {
		return false;
	}

	if (conn->required_sec_level >= BT_SECURITY_L4 &&
	    !((conn->le.keys->flags & BT_KEYS_AUTHENTICATED) &&
	      (conn->le.keys->keys & BT_KEYS_LTK_P256) &&
	      (conn->le.keys->enc_size == BT_SMP_MAX_ENC_KEY_SIZE))) {
		return false;
	}

	return true;
}

static uint8_t get_pair_method(struct bt_smp *smp, uint8_t remote_io)
{
#if !defined(CONFIG_BT_SMP_SC_PAIR_ONLY)
	if (!atomic_test_bit(smp->flags, SMP_FLAG_SC)) {
		return legacy_get_pair_method(smp, remote_io);
	}
#endif

#if !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)
	struct bt_smp_pairing *req, *rsp;

	req = (struct bt_smp_pairing *)&smp->preq[1];
	rsp = (struct bt_smp_pairing *)&smp->prsp[1];

	if ((req->auth_req & rsp->auth_req) & BT_SMP_AUTH_SC) {
		/* if one side has OOB data use OOB */
		if ((req->oob_flag | rsp->oob_flag) & BT_SMP_OOB_DATA_MASK) {
			return LE_SC_OOB;
		}
	}

	if (remote_io > BT_SMP_IO_KEYBOARD_DISPLAY) {
		return JUST_WORKS;
	}

	/* if none side requires MITM use JustWorks */
	if (!((req->auth_req | rsp->auth_req) & BT_SMP_AUTH_MITM)) {
		return JUST_WORKS;
	}

	return gen_method_sc[remote_io][get_io_capa(smp)];
#else
	return JUST_WORKS;
#endif
}

static enum bt_security_err security_err_get(uint8_t smp_err)
{
	switch (smp_err) {
	case BT_SMP_ERR_PASSKEY_ENTRY_FAILED:
	case BT_SMP_ERR_DHKEY_CHECK_FAILED:
	case BT_SMP_ERR_NUMERIC_COMP_FAILED:
	case BT_SMP_ERR_CONFIRM_FAILED:
		return BT_SECURITY_ERR_AUTH_FAIL;
	case BT_SMP_ERR_OOB_NOT_AVAIL:
		return BT_SECURITY_ERR_OOB_NOT_AVAILABLE;
	case BT_SMP_ERR_AUTH_REQUIREMENTS:
	case BT_SMP_ERR_ENC_KEY_SIZE:
		return BT_SECURITY_ERR_AUTH_REQUIREMENT;
	case BT_SMP_ERR_PAIRING_NOTSUPP:
	case BT_SMP_ERR_CMD_NOTSUPP:
		return BT_SECURITY_ERR_PAIR_NOT_SUPPORTED;
	case BT_SMP_ERR_REPEATED_ATTEMPTS:
	case BT_SMP_ERR_BREDR_PAIRING_IN_PROGRESS:
	case BT_SMP_ERR_CROSS_TRANSP_NOT_ALLOWED:
		return BT_SECURITY_ERR_PAIR_NOT_ALLOWED;
	case BT_SMP_ERR_INVALID_PARAMS:
		return BT_SECURITY_ERR_INVALID_PARAM;
	case BT_SMP_ERR_KEY_REJECTED:
		return BT_SECURITY_ERR_KEY_REJECTED;
	case BT_SMP_ERR_UNSPECIFIED:
	default:
		return BT_SECURITY_ERR_UNSPECIFIED;
	}
}

const char *bt_security_err_to_str(enum bt_security_err err)
{
	#define SEC_ERR(err) [err] = #err

	const char * const mapping_table[] = {
		SEC_ERR(BT_SECURITY_ERR_SUCCESS),
		SEC_ERR(BT_SECURITY_ERR_AUTH_FAIL),
		SEC_ERR(BT_SECURITY_ERR_PIN_OR_KEY_MISSING),
		SEC_ERR(BT_SECURITY_ERR_OOB_NOT_AVAILABLE),
		SEC_ERR(BT_SECURITY_ERR_AUTH_REQUIREMENT),
		SEC_ERR(BT_SECURITY_ERR_PAIR_NOT_SUPPORTED),
		SEC_ERR(BT_SECURITY_ERR_PAIR_NOT_ALLOWED),
		SEC_ERR(BT_SECURITY_ERR_INVALID_PARAM),
		SEC_ERR(BT_SECURITY_ERR_KEY_REJECTED),
		SEC_ERR(BT_SECURITY_ERR_UNSPECIFIED),
	};

	if (err < ARRAY_SIZE(mapping_table) && mapping_table[err]) {
		return mapping_table[err];
	} else {
		return "(unknown)";
	}

	#undef SEC_ERR
}


static uint8_t smp_err_get(enum bt_security_err auth_err)
{
	switch (auth_err) {
	case BT_SECURITY_ERR_OOB_NOT_AVAILABLE:
		return BT_SMP_ERR_OOB_NOT_AVAIL;

	case BT_SECURITY_ERR_AUTH_FAIL:
	case BT_SECURITY_ERR_AUTH_REQUIREMENT:
		return BT_SMP_ERR_AUTH_REQUIREMENTS;

	case BT_SECURITY_ERR_PAIR_NOT_SUPPORTED:
		return BT_SMP_ERR_PAIRING_NOTSUPP;

	case BT_SECURITY_ERR_INVALID_PARAM:
		return BT_SMP_ERR_INVALID_PARAMS;

	case BT_SECURITY_ERR_PIN_OR_KEY_MISSING:
	case BT_SECURITY_ERR_PAIR_NOT_ALLOWED:
	case BT_SECURITY_ERR_UNSPECIFIED:
		return BT_SMP_ERR_UNSPECIFIED;
	default:
		return 0;
	}
}

const char *bt_smp_err_to_str(uint8_t smp_err)
{
	#define SMP_ERR(err) [err] = #err

	const char * const mapping_table[] = {
		SMP_ERR(BT_SMP_ERR_SUCCESS),
		SMP_ERR(BT_SMP_ERR_PASSKEY_ENTRY_FAILED),
		SMP_ERR(BT_SMP_ERR_OOB_NOT_AVAIL),
		SMP_ERR(BT_SMP_ERR_AUTH_REQUIREMENTS),
		SMP_ERR(BT_SMP_ERR_CONFIRM_FAILED),
		SMP_ERR(BT_SMP_ERR_PAIRING_NOTSUPP),
		SMP_ERR(BT_SMP_ERR_ENC_KEY_SIZE),
		SMP_ERR(BT_SMP_ERR_CMD_NOTSUPP),
		SMP_ERR(BT_SMP_ERR_UNSPECIFIED),
		SMP_ERR(BT_SMP_ERR_REPEATED_ATTEMPTS),
		SMP_ERR(BT_SMP_ERR_INVALID_PARAMS),
		SMP_ERR(BT_SMP_ERR_DHKEY_CHECK_FAILED),
		SMP_ERR(BT_SMP_ERR_NUMERIC_COMP_FAILED),
		SMP_ERR(BT_SMP_ERR_BREDR_PAIRING_IN_PROGRESS),
		SMP_ERR(BT_SMP_ERR_CROSS_TRANSP_NOT_ALLOWED),
		SMP_ERR(BT_SMP_ERR_KEY_REJECTED),
	};

	if (smp_err < ARRAY_SIZE(mapping_table) && mapping_table[smp_err]) {
		return mapping_table[smp_err];
	} else {
		return "(unknown)";
	}

	#undef SMP_ERR
}

static struct net_buf *smp_create_pdu(struct bt_smp *smp, uint8_t op, size_t len)
{
	struct bt_smp_hdr *hdr;
	struct net_buf *buf;
	k_timeout_t timeout;

	/* Don't if session had already timed out */
	if (atomic_test_bit(smp->flags, SMP_FLAG_TIMEOUT)) {
		timeout = K_NO_WAIT;
	} else {
		timeout = SMP_TIMEOUT;
	}

	/* Use smaller timeout if returning an error since that could be
	 * caused by lack of buffers.
	 */
	buf = bt_l2cap_create_pdu_timeout(NULL, 0, timeout);
	if (!buf) {
		/* If it was not possible to allocate a buffer within the
		 * timeout marked it as timed out.
		 */
		atomic_set_bit(smp->flags, SMP_FLAG_TIMEOUT);
		return NULL;
	}

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->code = op;

	return buf;
}

static uint8_t get_encryption_key_size(struct bt_smp *smp)
{
	struct bt_smp_pairing *req, *rsp;

	req = (struct bt_smp_pairing *)&smp->preq[1];
	rsp = (struct bt_smp_pairing *)&smp->prsp[1];

	/*
	 * The smaller value of the initiating and responding devices maximum
	 * encryption key length parameters shall be used as the encryption key
	 * size.
	 */
	return MIN(req->max_key_size, rsp->max_key_size);
}

/* Check that if a new pairing procedure with an existing bond will not lower
 * the established security level of the bond.
 */
static bool update_keys_check(struct bt_smp *smp, struct bt_keys *keys)
{
	if (IS_ENABLED(CONFIG_BT_SMP_DISABLE_LEGACY_JW_PASSKEY) &&
	    !atomic_test_bit(smp->flags, SMP_FLAG_SC) &&
	    smp->method != LEGACY_OOB) {
		return false;
	}

	if (IS_ENABLED(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY) &&
	    smp->method != LEGACY_OOB) {
		return false;
	}

	if (!keys ||
	    !(keys->keys & (BT_KEYS_LTK_P256 | BT_KEYS_LTK))) {
		return true;
	}

	if (keys->enc_size > get_encryption_key_size(smp)) {
		return false;
	}

	if ((keys->keys & BT_KEYS_LTK_P256) &&
	    !atomic_test_bit(smp->flags, SMP_FLAG_SC)) {
		return false;
	}

	if ((keys->flags & BT_KEYS_AUTHENTICATED) &&
	     smp->method == JUST_WORKS) {
		return false;
	}

	if (!IS_ENABLED(CONFIG_BT_SMP_ALLOW_UNAUTH_OVERWRITE) &&
	    (!(keys->flags & BT_KEYS_AUTHENTICATED)
	     && smp->method == JUST_WORKS)) {
		if (!IS_ENABLED(CONFIG_BT_ID_ALLOW_UNAUTH_OVERWRITE) ||
		    (keys->id == smp->chan.chan.conn->id)) {
			return false;
		}
	}

	return true;
}

#ifndef CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY
static bool update_debug_keys_check(struct bt_smp *smp)
{
	struct bt_conn *conn = smp->chan.chan.conn;

	if (!conn->le.keys) {
		conn->le.keys = bt_keys_get_addr(conn->id, &conn->le.dst);
	}

	if (!conn->le.keys ||
	    !(conn->le.keys->keys & (BT_KEYS_LTK_P256 | BT_KEYS_LTK))) {
		return true;
	}

	if (conn->le.keys->flags & BT_KEYS_DEBUG) {
		return true;
	}

	return false;
}
#endif /* CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY */

#if defined(CONFIG_BT_PRIVACY) || defined(CONFIG_BT_SIGNING) || \
	!defined(CONFIG_BT_SMP_SC_PAIR_ONLY)
/* For TX callbacks */
static void smp_pairing_complete(struct bt_smp *smp, uint8_t status);
#if defined(CONFIG_BT_CLASSIC)
static void smp_pairing_br_complete(struct bt_smp_br *smp, uint8_t status);
#endif

static void smp_check_complete(struct bt_conn *conn, uint8_t dist_complete)
{
	struct bt_l2cap_chan *chan;

	if (conn->type == BT_CONN_TYPE_LE) {
		struct bt_smp *smp;

		chan = bt_l2cap_le_lookup_tx_cid(conn, BT_L2CAP_CID_SMP);
		__ASSERT(chan, "No SMP channel found");

		smp = CONTAINER_OF(chan, struct bt_smp, chan.chan);
		smp->local_dist &= ~dist_complete;

		/* if all keys were distributed, pairing is done */
		if (!smp->local_dist && !smp->remote_dist) {
			smp_pairing_complete(smp, 0);
		}

		return;
	}

#if defined(CONFIG_BT_CLASSIC)
	if (conn->type == BT_CONN_TYPE_BR) {
		struct bt_smp_br *smp;

		chan = bt_l2cap_le_lookup_tx_cid(conn, BT_L2CAP_CID_BR_SMP);
		__ASSERT(chan, "No SMP channel found");

		smp = CONTAINER_OF(chan, struct bt_smp_br, chan.chan);
		smp->local_dist &= ~dist_complete;

		/* if all keys were distributed, pairing is done */
		if (!smp->local_dist && !smp->remote_dist) {
			smp_pairing_br_complete(smp, 0);
		}
	}
#endif
}
#endif

#if defined(CONFIG_BT_PRIVACY)
static void smp_id_sent(struct bt_conn *conn, void *user_data, int err)
{
	if (!err) {
		smp_check_complete(conn, BT_SMP_DIST_ID_KEY);
	}
}
#endif /* CONFIG_BT_PRIVACY */

#if defined(CONFIG_BT_SIGNING)
static void smp_sign_info_sent(struct bt_conn *conn, void *user_data, int err)
{
	if (!err) {
		smp_check_complete(conn, BT_SMP_DIST_SIGN);
	}
}
#endif /* CONFIG_BT_SIGNING */

#if defined(CONFIG_BT_CLASSIC)
static void sc_derive_link_key(struct bt_smp *smp)
{
	/* constants as specified in Core Spec Vol.3 Part H 2.4.2.4 */
	static const uint8_t lebr[4] = { 0x72, 0x62, 0x65, 0x6c };
	struct bt_conn *conn = smp->chan.chan.conn;
	struct bt_keys_link_key *link_key;
	uint8_t ilk[16];

	LOG_DBG("");

	/* TODO handle errors? */

	/*
	 * At this point remote device identity is known so we can use
	 * destination address here
	 */
	link_key = bt_keys_get_link_key(&conn->le.dst.a);
	if (!link_key) {
		return;
	}

	if (atomic_test_bit(smp->flags, SMP_FLAG_CT2)) {
		/* constants as specified in Core Spec Vol.3 Part H 2.4.2.4 */
		static const uint8_t salt[16] = { 0x31, 0x70, 0x6d, 0x74,
					       0x00, 0x00, 0x00, 0x00,
					       0x00, 0x00, 0x00, 0x00,
					       0x00, 0x00, 0x00, 0x00 };

		if (bt_crypto_h7(salt, conn->le.keys->ltk.val, ilk)) {
			bt_keys_link_key_clear(link_key);
			return;
		}
	} else {
		/* constants as specified in Core Spec Vol.3 Part H 2.4.2.4 */
		static const uint8_t tmp1[4] = { 0x31, 0x70, 0x6d, 0x74 };

		if (bt_crypto_h6(conn->le.keys->ltk.val, tmp1, ilk)) {
			bt_keys_link_key_clear(link_key);
			return;
		}
	}

	if (bt_crypto_h6(ilk, lebr, link_key->val)) {
		bt_keys_link_key_clear(link_key);
	}

	link_key->flags |= BT_LINK_KEY_SC;

	if (conn->le.keys->flags & BT_KEYS_AUTHENTICATED) {
		link_key->flags |= BT_LINK_KEY_AUTHENTICATED;
	} else {
		link_key->flags &= ~BT_LINK_KEY_AUTHENTICATED;
	}
}

static void smp_br_reset(struct bt_smp_br *smp)
{
	/* Clear flags first in case canceling of timeout fails. The SMP context
	 * shall be marked as timed out in that case.
	 */
	atomic_set(smp->flags, 0);

	/* If canceling fails the timeout handler will set the timeout flag and
	 * mark the it as timed out. No new pairing procedures shall be started
	 * on this connection if that happens.
	 */
	(void)k_work_cancel_delayable(&smp->work);

	atomic_set(smp->allowed_cmds, 0);

	atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_REQ);
}

static void smp_pairing_br_complete(struct bt_smp_br *smp, uint8_t status)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	struct bt_keys *keys;
	bt_addr_le_t addr;

	LOG_DBG("status 0x%x", status);

	/* For dualmode devices LE address is same as BR/EDR address
	 * and is of public type.
	 */
	bt_addr_copy(&addr.a, &conn->br.dst);
	addr.type = BT_ADDR_LE_PUBLIC;
	keys = bt_keys_find_addr(conn->id, &addr);

	if (status) {
		struct bt_conn_auth_info_cb *listener, *next;

		if (keys) {
			bt_keys_clear(keys);
		}

		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&bt_auth_info_cbs, listener,
						  next, node) {
			if (listener->pairing_failed) {
				listener->pairing_failed(smp->chan.chan.conn,
							 security_err_get(status));
			}
		}
	} else {
		bool bond_flag = atomic_test_bit(smp->flags, SMP_FLAG_BOND);
		struct bt_conn_auth_info_cb *listener, *next;

		if (bond_flag && keys) {
			bt_keys_store(keys);
		}

		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&bt_auth_info_cbs, listener,
						  next, node) {
			if (listener->pairing_complete) {
				listener->pairing_complete(smp->chan.chan.conn,
							   bond_flag);
			}
		}
	}

	smp_br_reset(smp);
}

static void smp_br_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bt_smp_br *smp = CONTAINER_OF(dwork, struct bt_smp_br, work);

	LOG_ERR("SMP Timeout");

	smp_pairing_br_complete(smp, BT_SMP_ERR_UNSPECIFIED);
	atomic_set_bit(smp->flags, SMP_FLAG_TIMEOUT);
}

static void smp_br_send(struct bt_smp_br *smp, struct net_buf *buf,
			bt_conn_tx_cb_t cb)
{
	int err = bt_l2cap_br_send_cb(smp->chan.chan.conn, BT_L2CAP_CID_BR_SMP, buf, cb, NULL);

	if (err) {
		if (err == -ENOBUFS) {
			LOG_ERR("Ran out of TX buffers or contexts.");
		}

		net_buf_unref(buf);
		return;
	}

	k_work_reschedule(&smp->work, SMP_TIMEOUT);
}

static void bt_smp_br_connected(struct bt_l2cap_chan *chan)
{
	struct bt_smp_br *smp = CONTAINER_OF(chan, struct bt_smp_br, chan.chan);

	LOG_DBG("chan %p cid 0x%04x", chan,
		CONTAINER_OF(chan, struct bt_l2cap_br_chan, chan)->tx.cid);

	atomic_set_bit(smp->flags, SMP_FLAG_BR_CONNECTED);

	/*
	 * if this flag is set it means pairing was requested before channel
	 * was connected
	 */
	if (atomic_test_bit(smp->flags, SMP_FLAG_BR_PAIR)) {
		bt_smp_br_send_pairing_req(chan->conn);
	}
}

static void bt_smp_br_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_smp_br *smp = CONTAINER_OF(chan, struct bt_smp_br, chan.chan);

	LOG_DBG("chan %p cid 0x%04x", chan,
		CONTAINER_OF(chan, struct bt_l2cap_br_chan, chan)->tx.cid);

	/* Channel disconnected callback is always called from a work handler
	 * so canceling of the timeout work should always succeed.
	 */
	(void)k_work_cancel_delayable(&smp->work);

	(void)memset(smp, 0, sizeof(*smp));
}

static void smp_br_init(struct bt_smp_br *smp)
{
	/* Initialize SMP context excluding L2CAP channel context and anything
	 * else declared after.
	 */
	(void)memset(smp, 0, offsetof(struct bt_smp_br, chan));

	atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_FAIL);
}

static void smp_br_derive_ltk(struct bt_smp_br *smp)
{
	/* constants as specified in Core Spec Vol.3 Part H 2.4.2.5 */
	static const uint8_t brle[4] = { 0x65, 0x6c, 0x72, 0x62 };
	struct bt_conn *conn = smp->chan.chan.conn;
	struct bt_keys_link_key *link_key = conn->br.link_key;
	struct bt_keys *keys;
	bt_addr_le_t addr;
	uint8_t ilk[16];

	LOG_DBG("");

	if (!link_key) {
		return;
	}

	if (IS_ENABLED(CONFIG_BT_SMP_FORCE_BREDR) && conn->encrypt != 0x02) {
		LOG_WRN("Using P192 Link Key for P256 LTK derivation");
	}

	/*
	 * For dualmode devices LE address is same as BR/EDR address and is of
	 * public type.
	 */
	bt_addr_copy(&addr.a, &conn->br.dst);
	addr.type = BT_ADDR_LE_PUBLIC;

	keys = bt_keys_get_type(BT_KEYS_LTK_P256, conn->id, &addr);
	if (!keys) {
		LOG_ERR("Unable to get keys for %s", bt_addr_le_str(&addr));
		return;
	}

	if (atomic_test_bit(smp->flags, SMP_FLAG_CT2)) {
		/* constants as specified in Core Spec Vol.3 Part H 2.4.2.5 */
		static const uint8_t salt[16] = { 0x32, 0x70, 0x6d, 0x74,
					       0x00, 0x00, 0x00, 0x00,
					       0x00, 0x00, 0x00, 0x00,
					       0x00, 0x00, 0x00, 0x00 };

		if (bt_crypto_h7(salt, link_key->val, ilk)) {
			bt_keys_link_key_clear(link_key);
			return;
		}
	} else {
		/* constants as specified in Core Spec Vol.3 Part H 2.4.2.5 */
		static const uint8_t tmp2[4] = { 0x32, 0x70, 0x6d, 0x74 };

		if (bt_crypto_h6(link_key->val, tmp2, ilk)) {
			bt_keys_clear(keys);
			return;
		}
	}

	if (bt_crypto_h6(ilk, brle, keys->ltk.val)) {
		bt_keys_clear(keys);
		return;
	}

	(void)memset(keys->ltk.ediv, 0, sizeof(keys->ltk.ediv));
	(void)memset(keys->ltk.rand, 0, sizeof(keys->ltk.rand));
	keys->enc_size = smp->enc_key_size;

	if (link_key->flags & BT_LINK_KEY_AUTHENTICATED) {
		keys->flags |= BT_KEYS_AUTHENTICATED;
	} else {
		keys->flags &= ~BT_KEYS_AUTHENTICATED;
	}

	LOG_DBG("LTK derived from LinkKey");
}

static struct net_buf *smp_br_create_pdu(struct bt_smp_br *smp, uint8_t op,
					 size_t len)
{
	struct bt_smp_hdr *hdr;
	struct net_buf *buf;
	k_timeout_t timeout;

	/* Don't if session had already timed out */
	if (atomic_test_bit(smp->flags, SMP_FLAG_TIMEOUT)) {
		timeout = K_NO_WAIT;
	} else {
		timeout = SMP_TIMEOUT;
	}

	/* Use smaller timeout if returning an error since that could be
	 * caused by lack of buffers.
	 */
	buf = bt_l2cap_create_pdu_timeout(NULL, 0, timeout);
	if (!buf) {
		/* If it was not possible to allocate a buffer within the
		 * timeout marked it as timed out.
		 */
		atomic_set_bit(smp->flags, SMP_FLAG_TIMEOUT);
		return NULL;
	}

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->code = op;

	return buf;
}

static void smp_br_distribute_keys(struct bt_smp_br *smp)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	struct bt_keys *keys;
	bt_addr_le_t addr;

	/*
	 * For dualmode devices LE address is same as BR/EDR address and is of
	 * public type.
	 */
	bt_addr_copy(&addr.a, &conn->br.dst);
	addr.type = BT_ADDR_LE_PUBLIC;

	keys = bt_keys_get_addr(conn->id, &addr);
	if (!keys) {
		LOG_ERR("No keys space for %s", bt_addr_le_str(&addr));
		return;
	}

#if defined(CONFIG_BT_PRIVACY)
	if (smp->local_dist & BT_SMP_DIST_ID_KEY) {
		struct bt_smp_ident_info *id_info;
		struct bt_smp_ident_addr_info *id_addr_info;
		struct net_buf *buf;

		smp->local_dist &= ~BT_SMP_DIST_ID_KEY;

		buf = smp_br_create_pdu(smp, BT_SMP_CMD_IDENT_INFO,
					sizeof(*id_info));
		if (!buf) {
			LOG_ERR("Unable to allocate Ident Info buffer");
			return;
		}

		id_info = net_buf_add(buf, sizeof(*id_info));
		memcpy(id_info->irk, bt_dev.irk[conn->id], 16);

		smp_br_send(smp, buf, NULL);

		buf = smp_br_create_pdu(smp, BT_SMP_CMD_IDENT_ADDR_INFO,
				     sizeof(*id_addr_info));
		if (!buf) {
			LOG_ERR("Unable to allocate Ident Addr Info buffer");
			return;
		}

		id_addr_info = net_buf_add(buf, sizeof(*id_addr_info));
		bt_addr_le_copy(&id_addr_info->addr, &bt_dev.id_addr[conn->id]);

		smp_br_send(smp, buf, smp_id_sent);
	}
#endif /* CONFIG_BT_PRIVACY */

#if defined(CONFIG_BT_SIGNING)
	if (smp->local_dist & BT_SMP_DIST_SIGN) {
		struct bt_smp_signing_info *info;
		struct net_buf *buf;

		smp->local_dist &= ~BT_SMP_DIST_SIGN;

		buf = smp_br_create_pdu(smp, BT_SMP_CMD_SIGNING_INFO,
					sizeof(*info));
		if (!buf) {
			LOG_ERR("Unable to allocate Signing Info buffer");
			return;
		}

		info = net_buf_add(buf, sizeof(*info));

		if (bt_rand(info->csrk, sizeof(info->csrk))) {
			LOG_ERR("Unable to get random bytes");
			return;
		}

		if (atomic_test_bit(smp->flags, SMP_FLAG_BOND)) {
			bt_keys_add_type(keys, BT_KEYS_LOCAL_CSRK);
			memcpy(keys->local_csrk.val, info->csrk, 16);
			keys->local_csrk.cnt = 0U;
		}

		smp_br_send(smp, buf, smp_sign_info_sent);
	}
#endif /* CONFIG_BT_SIGNING */
}

static bool smp_br_pairing_allowed(struct bt_smp_br *smp)
{
	if (smp->chan.chan.conn->encrypt == 0x02) {
		return true;
	}

	if (IS_ENABLED(CONFIG_BT_SMP_FORCE_BREDR) &&
	    smp->chan.chan.conn->encrypt == 0x01) {
		LOG_WRN("Allowing BR/EDR SMP with P-192 key");
		return true;
	}

	return false;
}

static uint8_t smp_br_pairing_req(struct bt_smp_br *smp, struct net_buf *buf)
{
	struct bt_smp_pairing *req = (void *)buf->data;
	struct bt_conn *conn = smp->chan.chan.conn;
	struct bt_smp_pairing *rsp;
	struct net_buf *rsp_buf;
	uint8_t max_key_size;

	LOG_DBG("req: io_capability 0x%02X, oob_flag 0x%02X, auth_req 0x%02X, "
		"max_key_size 0x%02X, init_key_dist 0x%02X, resp_key_dist 0x%02X",
		req->io_capability, req->oob_flag, req->auth_req,
		req->max_key_size, req->init_key_dist, req->resp_key_dist);

	/*
	 * If a Pairing Request is received over the BR/EDR transport when
	 * either cross-transport key derivation/generation is not supported or
	 * the BR/EDR transport is not encrypted using a Link Key generated
	 * using P256, a Pairing Failed shall be sent with the error code
	 * "Cross-transport Key Derivation/Generation not allowed" (0x0E)."
	 */
	if (!smp_br_pairing_allowed(smp)) {
		return BT_SMP_ERR_CROSS_TRANSP_NOT_ALLOWED;
	}

	max_key_size = bt_conn_enc_key_size(conn);
	if (!max_key_size) {
		LOG_DBG("Invalid encryption key size");
		return BT_SMP_ERR_UNSPECIFIED;
	}

	if (req->max_key_size != max_key_size) {
		return BT_SMP_ERR_ENC_KEY_SIZE;
	}

	rsp_buf = smp_br_create_pdu(smp, BT_SMP_CMD_PAIRING_RSP, sizeof(*rsp));
	if (!rsp_buf) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	smp_br_init(smp);
	smp->enc_key_size = max_key_size;

	/*
	 * If Secure Connections pairing has been initiated over BR/EDR, the IO
	 * Capability, OOB data flag and Auth Req fields of the SM Pairing
	 * Request/Response PDU shall be set to zero on transmission, and
	 * ignored on reception.
	 */
	rsp = net_buf_add(rsp_buf, sizeof(*rsp));

	rsp->auth_req = 0x00;
	rsp->io_capability = 0x00;
	rsp->oob_flag = 0x00;
	rsp->max_key_size = max_key_size;
	rsp->init_key_dist = (req->init_key_dist & BR_RECV_KEYS_SC);
	rsp->resp_key_dist = (req->resp_key_dist & BR_RECV_KEYS_SC);

	smp->local_dist = rsp->resp_key_dist;
	smp->remote_dist = rsp->init_key_dist;

	LOG_DBG("rsp: io_capability 0x%02X, oob_flag 0x%02X, auth_req 0x%02X, "
		"max_key_size 0x%02X, init_key_dist 0x%02X, resp_key_dist 0x%02X",
		rsp->io_capability, rsp->oob_flag, rsp->auth_req,
		rsp->max_key_size, rsp->init_key_dist, rsp->resp_key_dist);

	smp_br_send(smp, rsp_buf, NULL);

	atomic_set_bit(smp->flags, SMP_FLAG_PAIRING);

	/* derive LTK if requested and clear distribution bits */
	if ((smp->local_dist & BT_SMP_DIST_ENC_KEY) &&
	    (smp->remote_dist & BT_SMP_DIST_ENC_KEY)) {
		smp_br_derive_ltk(smp);
	}
	smp->local_dist &= ~BT_SMP_DIST_ENC_KEY;
	smp->remote_dist &= ~BT_SMP_DIST_ENC_KEY;

	/* BR/EDR acceptor is like LE Peripheral and distributes keys first */
	smp_br_distribute_keys(smp);

	if (smp->remote_dist & BT_SMP_DIST_ID_KEY) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_IDENT_INFO);
	} else if (smp->remote_dist & BT_SMP_DIST_SIGN) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_SIGNING_INFO);
	}

	/* if all keys were distributed, pairing is done */
	if (!smp->local_dist && !smp->remote_dist) {
		smp_pairing_br_complete(smp, 0);
	}

	return 0;
}

static uint8_t smp_br_pairing_rsp(struct bt_smp_br *smp, struct net_buf *buf)
{
	struct bt_smp_pairing *rsp = (void *)buf->data;
	struct bt_conn *conn = smp->chan.chan.conn;
	uint8_t max_key_size;

	LOG_DBG("rsp: io_capability 0x%02X, oob_flag 0x%02X, auth_req 0x%02X, "
		"max_key_size 0x%02X, init_key_dist 0x%02X, resp_key_dist 0x%02X",
		rsp->io_capability, rsp->oob_flag, rsp->auth_req,
		rsp->max_key_size, rsp->init_key_dist, rsp->resp_key_dist);

	max_key_size = bt_conn_enc_key_size(conn);
	if (!max_key_size) {
		LOG_DBG("Invalid encryption key size");
		return BT_SMP_ERR_UNSPECIFIED;
	}

	if (rsp->max_key_size != max_key_size) {
		return BT_SMP_ERR_ENC_KEY_SIZE;
	}

	smp->local_dist &= rsp->init_key_dist;
	smp->remote_dist &= rsp->resp_key_dist;

	smp->local_dist &= SEND_KEYS_SC;
	smp->remote_dist &= RECV_KEYS_SC;

	/* Peripheral distributes its keys first */

	if (smp->remote_dist & BT_SMP_DIST_ID_KEY) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_IDENT_INFO);
	} else if (smp->remote_dist & BT_SMP_DIST_SIGN) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_SIGNING_INFO);
	}

	/* derive LTK if requested and clear distribution bits */
	if ((smp->local_dist & BT_SMP_DIST_ENC_KEY) &&
	    (smp->remote_dist & BT_SMP_DIST_ENC_KEY)) {
		smp_br_derive_ltk(smp);
	}
	smp->local_dist &= ~BT_SMP_DIST_ENC_KEY;
	smp->remote_dist &= ~BT_SMP_DIST_ENC_KEY;

	/* Pairing acceptor distributes it's keys first */
	if (smp->remote_dist) {
		return 0;
	}

	smp_br_distribute_keys(smp);

	/* if all keys were distributed, pairing is done */
	if (!smp->local_dist && !smp->remote_dist) {
		smp_pairing_br_complete(smp, 0);
	}

	return 0;
}

static uint8_t smp_br_pairing_failed(struct bt_smp_br *smp, struct net_buf *buf)
{
	struct bt_smp_pairing_fail *req = (void *)buf->data;

	LOG_ERR("pairing failed (peer reason 0x%x)", req->reason);

	smp_pairing_br_complete(smp, req->reason);
	smp_br_reset(smp);

	/* return no error to avoid sending Pairing Failed in response */
	return 0;
}

static uint8_t smp_br_ident_info(struct bt_smp_br *smp, struct net_buf *buf)
{
	struct bt_smp_ident_info *req = (void *)buf->data;
	struct bt_conn *conn = smp->chan.chan.conn;
	struct bt_keys *keys;
	bt_addr_le_t addr;

	LOG_DBG("");

	/* TODO should we resolve LE address if matching RPA is connected? */

	/*
	 * For dualmode devices LE address is same as BR/EDR address and is of
	 * public type.
	 */
	bt_addr_copy(&addr.a, &conn->br.dst);
	addr.type = BT_ADDR_LE_PUBLIC;

	keys = bt_keys_get_type(BT_KEYS_IRK, conn->id, &addr);
	if (!keys) {
		LOG_ERR("Unable to get keys for %s", bt_addr_le_str(&addr));
		return BT_SMP_ERR_UNSPECIFIED;
	}

	memcpy(keys->irk.val, req->irk, sizeof(keys->irk.val));

	atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_IDENT_ADDR_INFO);

	return 0;
}

static uint8_t smp_br_ident_addr_info(struct bt_smp_br *smp,
				      struct net_buf *buf)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	struct bt_smp_ident_addr_info *req = (void *)buf->data;
	bt_addr_le_t addr;

	LOG_DBG("identity %s", bt_addr_le_str(&req->addr));

	/*
	 * For dual mode device identity address must be same as BR/EDR address
	 * and be of public type. So if received one doesn't match BR/EDR
	 * address we fail.
	 */

	bt_addr_copy(&addr.a, &conn->br.dst);
	addr.type = BT_ADDR_LE_PUBLIC;

	if (!bt_addr_le_eq(&addr, &req->addr)) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	smp->remote_dist &= ~BT_SMP_DIST_ID_KEY;

	if (smp->remote_dist & BT_SMP_DIST_SIGN) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_SIGNING_INFO);
	}

	if (conn->role == BT_CONN_ROLE_CENTRAL && !smp->remote_dist) {
		smp_br_distribute_keys(smp);
	}

	/* if all keys were distributed, pairing is done */
	if (!smp->local_dist && !smp->remote_dist) {
		smp_pairing_br_complete(smp, 0);
	}

	return 0;
}

#if defined(CONFIG_BT_SIGNING)
static uint8_t smp_br_signing_info(struct bt_smp_br *smp, struct net_buf *buf)
{
	struct bt_smp_signing_info *req = (void *)buf->data;
	struct bt_conn *conn = smp->chan.chan.conn;
	struct bt_keys *keys;
	bt_addr_le_t addr;

	LOG_DBG("");

	/*
	 * For dualmode devices LE address is same as BR/EDR address and is of
	 * public type.
	 */
	bt_addr_copy(&addr.a, &conn->br.dst);
	addr.type = BT_ADDR_LE_PUBLIC;

	keys = bt_keys_get_type(BT_KEYS_REMOTE_CSRK, conn->id, &addr);
	if (!keys) {
		LOG_ERR("Unable to get keys for %s", bt_addr_le_str(&addr));
		return BT_SMP_ERR_UNSPECIFIED;
	}

	memcpy(keys->remote_csrk.val, req->csrk, sizeof(keys->remote_csrk.val));

	smp->remote_dist &= ~BT_SMP_DIST_SIGN;

	if (conn->role == BT_CONN_ROLE_CENTRAL && !smp->remote_dist) {
		smp_br_distribute_keys(smp);
	}

	/* if all keys were distributed, pairing is done */
	if (!smp->local_dist && !smp->remote_dist) {
		smp_pairing_br_complete(smp, 0);
	}

	return 0;
}
#else
static uint8_t smp_br_signing_info(struct bt_smp_br *smp, struct net_buf *buf)
{
	return BT_SMP_ERR_CMD_NOTSUPP;
}
#endif /* CONFIG_BT_SIGNING */

static const struct {
	uint8_t  (*func)(struct bt_smp_br *smp, struct net_buf *buf);
	uint8_t  expect_len;
} br_handlers[] = {
	{ }, /* No op-code defined for 0x00 */
	{ smp_br_pairing_req,      sizeof(struct bt_smp_pairing) },
	{ smp_br_pairing_rsp,      sizeof(struct bt_smp_pairing) },
	{ }, /* pairing confirm not used over BR/EDR */
	{ }, /* pairing random not used over BR/EDR */
	{ smp_br_pairing_failed,   sizeof(struct bt_smp_pairing_fail) },
	{ }, /* encrypt info not used over BR/EDR */
	{ }, /* central ident not used over BR/EDR */
	{ smp_br_ident_info,       sizeof(struct bt_smp_ident_info) },
	{ smp_br_ident_addr_info,  sizeof(struct bt_smp_ident_addr_info) },
	{ smp_br_signing_info,     sizeof(struct bt_smp_signing_info) },
	/* security request not used over BR/EDR */
	/* public key not used over BR/EDR */
	/* DHKey check not used over BR/EDR */
};

static int smp_br_error(struct bt_smp_br *smp, uint8_t reason)
{
	struct bt_smp_pairing_fail *rsp;
	struct net_buf *buf;

	/* reset context and report */
	smp_br_reset(smp);

	buf = smp_br_create_pdu(smp, BT_SMP_CMD_PAIRING_FAIL, sizeof(*rsp));
	if (!buf) {
		return -ENOBUFS;
	}

	rsp = net_buf_add(buf, sizeof(*rsp));
	rsp->reason = reason;

	/*
	 * SMP timer is not restarted for PairingFailed so don't use
	 * smp_br_send
	 */
	if (bt_l2cap_br_send_cb(smp->chan.chan.conn, BT_L2CAP_CID_SMP, buf,
				NULL, NULL)) {
		net_buf_unref(buf);
	}

	return 0;
}

static int bt_smp_br_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_smp_br *smp = CONTAINER_OF(chan, struct bt_smp_br, chan.chan);
	struct bt_smp_hdr *hdr;
	uint8_t err;

	if (buf->len < sizeof(*hdr)) {
		LOG_ERR("Too small SMP PDU received");
		return 0;
	}

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	LOG_DBG("Received SMP code 0x%02x len %u", hdr->code, buf->len);

	/*
	 * If SMP timeout occurred "no further SMP commands shall be sent over
	 * the L2CAP Security Manager Channel. A new SM procedure shall only be
	 * performed when a new physical link has been established."
	 */
	if (atomic_test_bit(smp->flags, SMP_FLAG_TIMEOUT)) {
		LOG_WRN("SMP command (code 0x%02x) received after timeout", hdr->code);
		return 0;
	}

	if (hdr->code >= ARRAY_SIZE(br_handlers) ||
	    !br_handlers[hdr->code].func) {
		LOG_WRN("Unhandled SMP code 0x%02x", hdr->code);
		smp_br_error(smp, BT_SMP_ERR_CMD_NOTSUPP);
		return 0;
	}

	if (!atomic_test_and_clear_bit(smp->allowed_cmds, hdr->code)) {
		LOG_WRN("Unexpected SMP code 0x%02x", hdr->code);
		smp_br_error(smp, BT_SMP_ERR_UNSPECIFIED);
		return 0;
	}

	if (buf->len != br_handlers[hdr->code].expect_len) {
		LOG_ERR("Invalid len %u for code 0x%02x", buf->len, hdr->code);
		smp_br_error(smp, BT_SMP_ERR_INVALID_PARAMS);
		return 0;
	}

	err = br_handlers[hdr->code].func(smp, buf);
	if (err) {
		smp_br_error(smp, err);
	}

	return 0;
}

static bool br_sc_supported(void)
{
	if (IS_ENABLED(CONFIG_BT_SMP_FORCE_BREDR)) {
		LOG_WRN("Enabling BR/EDR SMP without BR/EDR SC support");
		return true;
	}

	return BT_FEAT_SC(bt_dev.features);
}

static int bt_smp_br_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
{
	static const struct bt_l2cap_chan_ops ops = {
		.connected = bt_smp_br_connected,
		.disconnected = bt_smp_br_disconnected,
		.recv = bt_smp_br_recv,
	};
	int i;

	/* Check BR/EDR SC is supported */
	if (!br_sc_supported()) {
		return -ENOTSUP;
	}

	LOG_DBG("conn %p handle %u", conn, conn->handle);

	for (i = 0; i < ARRAY_SIZE(bt_smp_pool); i++) {
		struct bt_smp_br *smp = &bt_smp_br_pool[i];

		if (smp->chan.chan.conn) {
			continue;
		}

		smp->chan.chan.ops = &ops;

		*chan = &smp->chan.chan;

		k_work_init_delayable(&smp->work, smp_br_timeout);
		smp_br_reset(smp);

		return 0;
	}

	LOG_ERR("No available SMP context for conn %p", conn);

	return -ENOMEM;
}

static struct bt_smp_br *smp_br_chan_get(struct bt_conn *conn)
{
	struct bt_l2cap_chan *chan;

	chan = bt_l2cap_br_lookup_rx_cid(conn, BT_L2CAP_CID_BR_SMP);
	if (!chan) {
		LOG_ERR("Unable to find SMP channel");
		return NULL;
	}

	return CONTAINER_OF(chan, struct bt_smp_br, chan.chan);
}

int bt_smp_br_send_pairing_req(struct bt_conn *conn)
{
	struct bt_smp_pairing *req;
	struct net_buf *req_buf;
	uint8_t max_key_size;
	struct bt_smp_br *smp;

	smp = smp_br_chan_get(conn);
	if (!smp) {
		return -ENOTCONN;
	}

	/* SMP Timeout */
	if (atomic_test_bit(smp->flags, SMP_FLAG_TIMEOUT)) {
		return -EIO;
	}

	/* pairing is in progress */
	if (atomic_test_bit(smp->flags, SMP_FLAG_PAIRING)) {
		return -EBUSY;
	}

	/* check if we are allowed to start SMP over BR/EDR */
	if (!smp_br_pairing_allowed(smp)) {
		return 0;
	}

	/* Channel not yet connected, will start pairing once connected */
	if (!atomic_test_bit(smp->flags, SMP_FLAG_BR_CONNECTED)) {
		atomic_set_bit(smp->flags, SMP_FLAG_BR_PAIR);
		return 0;
	}

	max_key_size = bt_conn_enc_key_size(conn);
	if (!max_key_size) {
		LOG_DBG("Invalid encryption key size");
		return -EIO;
	}

	smp_br_init(smp);
	smp->enc_key_size = max_key_size;

	req_buf = smp_br_create_pdu(smp, BT_SMP_CMD_PAIRING_REQ, sizeof(*req));
	if (!req_buf) {
		return -ENOBUFS;
	}

	req = net_buf_add(req_buf, sizeof(*req));

	/*
	 * If Secure Connections pairing has been initiated over BR/EDR, the IO
	 * Capability, OOB data flag and Auth Req fields of the SM Pairing
	 * Request/Response PDU shall be set to zero on transmission, and
	 * ignored on reception.
	 */

	req->auth_req = 0x00;
	req->io_capability = 0x00;
	req->oob_flag = 0x00;
	req->max_key_size = max_key_size;
	req->init_key_dist = BR_SEND_KEYS_SC;
	req->resp_key_dist = BR_RECV_KEYS_SC;

	smp_br_send(smp, req_buf, NULL);

	smp->local_dist = BR_SEND_KEYS_SC;
	smp->remote_dist = BR_RECV_KEYS_SC;

	atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_RSP);

	atomic_set_bit(smp->flags, SMP_FLAG_PAIRING);

	return 0;
}
#endif /* CONFIG_BT_CLASSIC */

static void smp_reset(struct bt_smp *smp)
{
	struct bt_conn *conn = smp->chan.chan.conn;

	/* Clear flags first in case canceling of timeout fails. The SMP context
	 * shall be marked as timed out in that case.
	 */
	atomic_set(smp->flags, 0);

	/* If canceling fails the timeout handler will set the timeout flag and
	 * mark the it as timed out. No new pairing procedures shall be started
	 * on this connection if that happens.
	 */
	(void)k_work_cancel_delayable(&smp->work);

	smp->method = JUST_WORKS;
	atomic_set(smp->allowed_cmds, 0);

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    conn->role == BT_HCI_ROLE_CENTRAL) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_SECURITY_REQUEST);
		return;
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_REQ);
	}
}

static uint8_t hci_err_get(enum bt_security_err err)
{
	switch (err) {
	case BT_SECURITY_ERR_SUCCESS:
		return BT_HCI_ERR_SUCCESS;
	case BT_SECURITY_ERR_AUTH_FAIL:
		return BT_HCI_ERR_AUTH_FAIL;
	case BT_SECURITY_ERR_PIN_OR_KEY_MISSING:
		return BT_HCI_ERR_PIN_OR_KEY_MISSING;
	case BT_SECURITY_ERR_PAIR_NOT_SUPPORTED:
		return BT_HCI_ERR_PAIRING_NOT_SUPPORTED;
	case BT_SECURITY_ERR_PAIR_NOT_ALLOWED:
		return BT_HCI_ERR_PAIRING_NOT_ALLOWED;
	case BT_SECURITY_ERR_INVALID_PARAM:
		return BT_HCI_ERR_INVALID_PARAM;
	default:
		return BT_HCI_ERR_UNSPECIFIED;
	}
}

/* Note: This function not only does set the status but also calls smp_reset
 * at the end which clears any flags previously set.
 */
static void smp_pairing_complete(struct bt_smp *smp, uint8_t status)
{
	struct bt_conn *conn = smp->chan.chan.conn;

	LOG_DBG("got status 0x%x", status);

	if (conn->le.keys == NULL) {
		/* We can get here if the application calls `bt_unpair` in the
		 * `security_changed` callback.
		 */
		LOG_WRN("The in-progress pairing has been deleted!");
		status = BT_SMP_ERR_UNSPECIFIED;
	}

	if (!status) {
#if defined(CONFIG_BT_CLASSIC)
		/*
		 * Don't derive if Debug Keys are used.
		 * TODO should we allow this if BR/EDR is already connected?
		 */
		if (atomic_test_bit(smp->flags, SMP_FLAG_DERIVE_LK) &&
		    (!atomic_test_bit(smp->flags, SMP_FLAG_SC_DEBUG_KEY) ||
		     IS_ENABLED(CONFIG_BT_STORE_DEBUG_KEYS))) {
			sc_derive_link_key(smp);
		}
#endif /* CONFIG_BT_CLASSIC */
		bool bond_flag = atomic_test_bit(smp->flags, SMP_FLAG_BOND);
		struct bt_conn_auth_info_cb *listener, *next;

		if (IS_ENABLED(CONFIG_BT_LOG_SNIFFER_INFO)) {
			bt_keys_show_sniffer_info(conn->le.keys, NULL);
		}

		if (bond_flag && conn->le.keys) {
			bt_keys_store(conn->le.keys);
		}

		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&bt_auth_info_cbs, listener,
						  next, node) {
			if (listener->pairing_complete) {
				listener->pairing_complete(conn, bond_flag);
			}
		}
	} else {
		enum bt_security_err security_err = security_err_get(status);

		/* Clear the key pool entry in case of pairing failure if the
		 * keys already existed before the pairing procedure or the
		 * pairing failed during key distribution.
		 */
		if (conn->le.keys &&
		    (!conn->le.keys->enc_size ||
		     atomic_test_bit(smp->flags, SMP_FLAG_KEYS_DISTR))) {
			bt_keys_clear(conn->le.keys);
			conn->le.keys = NULL;
		}

		if (!atomic_test_bit(smp->flags, SMP_FLAG_KEYS_DISTR)) {
			bt_conn_security_changed(conn,
						 hci_err_get(security_err),
						 security_err);
		}

		/* Check SMP_FLAG_PAIRING as bt_conn_security_changed may
		 * have called the pairing_failed callback already.
		 */
		if (atomic_test_bit(smp->flags, SMP_FLAG_PAIRING)) {
			struct bt_conn_auth_info_cb *listener, *next;

			SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&bt_auth_info_cbs,
							  listener, next,
							  node) {
				if (listener->pairing_failed) {
					listener->pairing_failed(conn, security_err);
				}
			}
		}
	}

	smp_reset(smp);

	if (conn->state == BT_CONN_CONNECTED && conn->sec_level != conn->required_sec_level) {
		bt_smp_start_security(conn);
	}
}

static void smp_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bt_smp *smp = CONTAINER_OF(dwork, struct bt_smp, work);

	LOG_ERR("SMP Timeout");

	smp_pairing_complete(smp, BT_SMP_ERR_UNSPECIFIED);

	/* smp_pairing_complete clears flags so setting timeout flag must come
	 * after it.
	 */
	atomic_set_bit(smp->flags, SMP_FLAG_TIMEOUT);
}

static void smp_send(struct bt_smp *smp, struct net_buf *buf,
		     bt_conn_tx_cb_t cb, void *user_data)
{
	__ASSERT_NO_MSG(user_data == NULL);

	int err = bt_l2cap_send_pdu(&smp->chan, buf, cb, NULL);

	if (err) {
		if (err == -ENOBUFS) {
			LOG_ERR("Ran out of TX buffers or contexts.");
		}

		net_buf_unref(buf);
		return;
	}

	k_work_reschedule(&smp->work, SMP_TIMEOUT);
}

static int smp_error(struct bt_smp *smp, uint8_t reason)
{
	struct bt_smp_pairing_fail *rsp;
	struct net_buf *buf;
	bool remote_already_completed;

	/* By spec, SMP "pairing process" completes successfully when the last
	 * key to distribute is acknowledged at link-layer.
	 */
	remote_already_completed = (atomic_test_bit(smp->flags, SMP_FLAG_KEYS_DISTR) &&
				    !smp->local_dist && !smp->remote_dist);

	if (atomic_test_bit(smp->flags, SMP_FLAG_PAIRING) ||
	    atomic_test_bit(smp->flags, SMP_FLAG_ENC_PENDING) ||
	    atomic_test_bit(smp->flags, SMP_FLAG_SEC_REQ)) {
		/* reset context and report */
		smp_pairing_complete(smp, reason);
	}

	if (remote_already_completed) {
		LOG_WRN("SMP does not allow a pairing failure at this point. Known issue. "
			"Disconnecting instead.");
		/* We are probably here because we are, as a peripheral, rejecting a pairing based
		 * on the central's identity address information, but that was the last key to
		 * be transmitted. In that case, the pairing process is already completed.
		 * The SMP protocol states that the pairing process is completed the moment the
		 * peripheral link-layer confirmed the reception of the PDU with the last key.
		 */
		bt_conn_disconnect(smp->chan.chan.conn, BT_HCI_ERR_AUTH_FAIL);
		return 0;
	}

	buf = smp_create_pdu(smp, BT_SMP_CMD_PAIRING_FAIL, sizeof(*rsp));
	if (!buf) {
		return -ENOBUFS;
	}

	rsp = net_buf_add(buf, sizeof(*rsp));
	rsp->reason = reason;

	/* SMP timer is not restarted for PairingFailed so don't use smp_send */
	if (bt_l2cap_send_pdu(&smp->chan, buf, NULL, NULL)) {
		net_buf_unref(buf);
	}

	return 0;
}

static uint8_t smp_send_pairing_random(struct bt_smp *smp)
{
	struct bt_smp_pairing_random *req;
	struct net_buf *rsp_buf;

	rsp_buf = smp_create_pdu(smp, BT_SMP_CMD_PAIRING_RANDOM, sizeof(*req));
	if (!rsp_buf) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	req = net_buf_add(rsp_buf, sizeof(*req));
	memcpy(req->val, smp->prnd, sizeof(req->val));

	smp_send(smp, rsp_buf, NULL, NULL);

	return 0;
}

#if !defined(CONFIG_BT_SMP_SC_PAIR_ONLY)
static int smp_c1(const uint8_t k[16], const uint8_t r[16],
		  const uint8_t preq[7], const uint8_t pres[7],
		  const bt_addr_le_t *ia, const bt_addr_le_t *ra,
		  uint8_t enc_data[16])
{
	uint8_t p1[16], p2[16];
	int err;

	LOG_DBG("k %s", bt_hex(k, 16));
	LOG_DBG("r %s", bt_hex(r, 16));
	LOG_DBG("ia %s", bt_addr_le_str(ia));
	LOG_DBG("ra %s", bt_addr_le_str(ra));
	LOG_DBG("preq %s", bt_hex(preq, 7));
	LOG_DBG("pres %s", bt_hex(pres, 7));

	/* pres, preq, rat and iat are concatenated to generate p1 */
	p1[0] = ia->type;
	p1[1] = ra->type;
	memcpy(p1 + 2, preq, 7);
	memcpy(p1 + 9, pres, 7);

	LOG_DBG("p1 %s", bt_hex(p1, 16));

	/* c1 = e(k, e(k, r XOR p1) XOR p2) */

	/* Using enc_data as temporary output buffer */
	mem_xor_128(enc_data, r, p1);

	err = bt_encrypt_le(k, enc_data, enc_data);
	if (err) {
		return err;
	}

	/* ra is concatenated with ia and padding to generate p2 */
	memcpy(p2, ra->a.val, 6);
	memcpy(p2 + 6, ia->a.val, 6);
	(void)memset(p2 + 12, 0, 4);

	LOG_DBG("p2 %s", bt_hex(p2, 16));

	mem_xor_128(enc_data, p2, enc_data);

	return bt_encrypt_le(k, enc_data, enc_data);
}
#endif /* !CONFIG_BT_SMP_SC_PAIR_ONLY */

static uint8_t smp_send_pairing_confirm(struct bt_smp *smp)
{
	struct bt_smp_pairing_confirm *req;
	struct net_buf *buf;
	uint8_t r;

	switch (smp->method) {
	case PASSKEY_CONFIRM:
	case JUST_WORKS:
		r = 0U;
		break;
	case PASSKEY_DISPLAY:
	case PASSKEY_INPUT:
		/*
		 * In the Passkey Entry protocol, the most significant
		 * bit of Z is set equal to one and the least
		 * significant bit is made up from one bit of the
		 * passkey e.g. if the passkey bit is 1, then Z = 0x81
		 * and if the passkey bit is 0, then Z = 0x80.
		 */
		r = (smp->passkey >> smp->passkey_round) & 0x01;
		r |= 0x80;
		break;
	default:
		LOG_ERR("Unknown pairing method (%u)", smp->method);
		return BT_SMP_ERR_UNSPECIFIED;
	}

	buf = smp_create_pdu(smp, BT_SMP_CMD_PAIRING_CONFIRM, sizeof(*req));
	if (!buf) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	req = net_buf_add(buf, sizeof(*req));

	if (bt_crypto_f4(sc_public_key, smp->pkey, smp->prnd, r, req->val)) {
		net_buf_unref(buf);
		return BT_SMP_ERR_UNSPECIFIED;
	}

	smp_send(smp, buf, NULL, NULL);

	atomic_clear_bit(smp->flags, SMP_FLAG_CFM_DELAYED);

	return 0;
}

#if !defined(CONFIG_BT_SMP_SC_PAIR_ONLY)
static void smp_ident_sent(struct bt_conn *conn, void *user_data, int err)
{
	if (!err) {
		smp_check_complete(conn, BT_SMP_DIST_ENC_KEY);
	}
}

static void legacy_distribute_keys(struct bt_smp *smp)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	struct bt_keys *keys = conn->le.keys;

	if (smp->local_dist & BT_SMP_DIST_ENC_KEY) {
		struct bt_smp_encrypt_info *info;
		struct bt_smp_central_ident *ident;
		struct net_buf *buf;
		/* Use struct to get randomness in single call to bt_rand */
		struct {
			uint8_t key[16];
			uint8_t rand[8];
			uint8_t ediv[2];
		} rand;

		if (bt_rand((void *)&rand, sizeof(rand))) {
			LOG_ERR("Unable to get random bytes");
			return;
		}

		buf = smp_create_pdu(smp, BT_SMP_CMD_ENCRYPT_INFO,
				     sizeof(*info));
		if (!buf) {
			LOG_ERR("Unable to allocate Encrypt Info buffer");
			return;
		}

		info = net_buf_add(buf, sizeof(*info));

		/* distributed only enc_size bytes of key */
		memcpy(info->ltk, rand.key, keys->enc_size);
		if (keys->enc_size < sizeof(info->ltk)) {
			(void)memset(info->ltk + keys->enc_size, 0,
				     sizeof(info->ltk) - keys->enc_size);
		}

		smp_send(smp, buf, NULL, NULL);

		buf = smp_create_pdu(smp, BT_SMP_CMD_CENTRAL_IDENT,
				     sizeof(*ident));
		if (!buf) {
			LOG_ERR("Unable to allocate Central Ident buffer");
			return;
		}

		ident = net_buf_add(buf, sizeof(*ident));
		memcpy(ident->rand, rand.rand, sizeof(ident->rand));
		memcpy(ident->ediv, rand.ediv, sizeof(ident->ediv));

		smp_send(smp, buf, smp_ident_sent, NULL);

		if (atomic_test_bit(smp->flags, SMP_FLAG_BOND)) {
			bt_keys_add_type(keys, BT_KEYS_PERIPH_LTK);

			memcpy(keys->periph_ltk.val, rand.key,
			       sizeof(keys->periph_ltk.val));
			memcpy(keys->periph_ltk.rand, rand.rand,
			       sizeof(keys->periph_ltk.rand));
			memcpy(keys->periph_ltk.ediv, rand.ediv,
			       sizeof(keys->periph_ltk.ediv));
		}
	}
}
#endif /* !CONFIG_BT_SMP_SC_PAIR_ONLY */

static uint8_t bt_smp_distribute_keys(struct bt_smp *smp)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	struct bt_keys *keys = conn->le.keys;

	if (!keys) {
		LOG_ERR("No keys space for %s", bt_addr_le_str(&conn->le.dst));
		return BT_SMP_ERR_UNSPECIFIED;
	}

#if !defined(CONFIG_BT_SMP_SC_PAIR_ONLY)
	/* Distribute legacy pairing specific keys */
	if (!atomic_test_bit(smp->flags, SMP_FLAG_SC)) {
		legacy_distribute_keys(smp);
	}
#endif /* !CONFIG_BT_SMP_SC_PAIR_ONLY */

#if defined(CONFIG_BT_PRIVACY)
	if (smp->local_dist & BT_SMP_DIST_ID_KEY) {
		struct bt_smp_ident_info *id_info;
		struct bt_smp_ident_addr_info *id_addr_info;
		struct net_buf *buf;

		buf = smp_create_pdu(smp, BT_SMP_CMD_IDENT_INFO,
				     sizeof(*id_info));
		if (!buf) {
			LOG_ERR("Unable to allocate Ident Info buffer");
			return BT_SMP_ERR_UNSPECIFIED;
		}

		id_info = net_buf_add(buf, sizeof(*id_info));
		memcpy(id_info->irk, bt_dev.irk[conn->id], 16);

		smp_send(smp, buf, NULL, NULL);

		buf = smp_create_pdu(smp, BT_SMP_CMD_IDENT_ADDR_INFO,
				     sizeof(*id_addr_info));
		if (!buf) {
			LOG_ERR("Unable to allocate Ident Addr Info buffer");
			return BT_SMP_ERR_UNSPECIFIED;
		}

		id_addr_info = net_buf_add(buf, sizeof(*id_addr_info));
		bt_addr_le_copy(&id_addr_info->addr, &bt_dev.id_addr[conn->id]);

		smp_send(smp, buf, smp_id_sent, NULL);
	}
#endif /* CONFIG_BT_PRIVACY */

#if defined(CONFIG_BT_SIGNING)
	if (smp->local_dist & BT_SMP_DIST_SIGN) {
		struct bt_smp_signing_info *info;
		struct net_buf *buf;

		buf = smp_create_pdu(smp, BT_SMP_CMD_SIGNING_INFO,
				     sizeof(*info));
		if (!buf) {
			LOG_ERR("Unable to allocate Signing Info buffer");
			return BT_SMP_ERR_UNSPECIFIED;
		}

		info = net_buf_add(buf, sizeof(*info));

		if (bt_rand(info->csrk, sizeof(info->csrk))) {
			return BT_SMP_ERR_UNSPECIFIED;
		}

		if (atomic_test_bit(smp->flags, SMP_FLAG_BOND)) {
			bt_keys_add_type(keys, BT_KEYS_LOCAL_CSRK);
			memcpy(keys->local_csrk.val, info->csrk, 16);
			keys->local_csrk.cnt = 0U;
		}

		smp_send(smp, buf, smp_sign_info_sent, NULL);
	}
#endif /* CONFIG_BT_SIGNING */

	return 0;
}

#if defined(CONFIG_BT_PERIPHERAL)
static uint8_t send_pairing_rsp(struct bt_smp *smp)
{
	struct bt_smp_pairing *rsp;
	struct net_buf *rsp_buf;

	rsp_buf = smp_create_pdu(smp, BT_SMP_CMD_PAIRING_RSP, sizeof(*rsp));
	if (!rsp_buf) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	rsp = net_buf_add(rsp_buf, sizeof(*rsp));
	memcpy(rsp, smp->prsp + 1, sizeof(*rsp));

	smp_send(smp, rsp_buf, NULL, NULL);

	return 0;
}
#endif /* CONFIG_BT_PERIPHERAL */

static uint8_t smp_pairing_accept_query(struct bt_smp *smp, struct bt_smp_pairing *pairing)
{
#if defined(CONFIG_BT_SMP_APP_PAIRING_ACCEPT)
	const struct bt_conn_auth_cb *smp_auth_cb = latch_auth_cb(smp);
	struct bt_conn *conn = smp->chan.chan.conn;

	if (smp_auth_cb && smp_auth_cb->pairing_accept) {
		const struct bt_conn_pairing_feat feat = {
			.io_capability = pairing->io_capability,
			.oob_data_flag = pairing->oob_flag,
			.auth_req = pairing->auth_req,
			.max_enc_key_size = pairing->max_key_size,
			.init_key_dist = pairing->init_key_dist,
			.resp_key_dist = pairing->resp_key_dist
		};

		return smp_err_get(smp_auth_cb->pairing_accept(conn, &feat));
	}
#endif /* CONFIG_BT_SMP_APP_PAIRING_ACCEPT */
	return 0;
}

#if !defined(CONFIG_BT_SMP_SC_PAIR_ONLY)
static int smp_s1(const uint8_t k[16], const uint8_t r1[16],
		  const uint8_t r2[16], uint8_t out[16])
{
	/* The most significant 64-bits of r1 are discarded to generate
	 * r1' and the most significant 64-bits of r2 are discarded to
	 * generate r2'.
	 * r1' is concatenated with r2' to generate r' which is used as
	 * the 128-bit input parameter plaintextData to security function e:
	 *
	 *    r' = r1' || r2'
	 */
	memcpy(out, r2, 8);
	memcpy(out + 8, r1, 8);

	/* s1(k, r1 , r2) = e(k, r') */
	return bt_encrypt_le(k, out, out);
}

static uint8_t legacy_get_pair_method(struct bt_smp *smp, uint8_t remote_io)
{
	struct bt_smp_pairing *req, *rsp;
	uint8_t method;

	if (remote_io > BT_SMP_IO_KEYBOARD_DISPLAY) {
		return JUST_WORKS;
	}

	req = (struct bt_smp_pairing *)&smp->preq[1];
	rsp = (struct bt_smp_pairing *)&smp->prsp[1];

	/* if both sides have OOB data use OOB */
	if ((req->oob_flag & rsp->oob_flag) & BT_SMP_OOB_DATA_MASK) {
		return LEGACY_OOB;
	}

	/* if none side requires MITM use JustWorks */
	if (!((req->auth_req | rsp->auth_req) & BT_SMP_AUTH_MITM)) {
		return JUST_WORKS;
	}

	method = gen_method_legacy[remote_io][get_io_capa(smp)];

	/* if both sides have KeyboardDisplay capabilities, initiator displays
	 * and responder inputs
	 */
	if (method == PASSKEY_ROLE) {
		if (smp->chan.chan.conn->role == BT_HCI_ROLE_CENTRAL) {
			method = PASSKEY_DISPLAY;
		} else {
			method = PASSKEY_INPUT;
		}
	}

	return method;
}

static uint8_t legacy_request_tk(struct bt_smp *smp)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	const struct bt_conn_auth_cb *smp_auth_cb = latch_auth_cb(smp);
	struct bt_keys *keys;
	uint32_t passkey;

	/*
	 * Fail if we have keys that are stronger than keys that will be
	 * distributed in new pairing. This is to avoid replacing authenticated
	 * keys with unauthenticated ones.
	  */
	keys = bt_keys_find_addr(conn->id, &conn->le.dst);
	if (keys && (keys->flags & BT_KEYS_AUTHENTICATED) &&
	    smp->method == JUST_WORKS) {
		LOG_ERR("JustWorks failed, authenticated keys present");
		return BT_SMP_ERR_UNSPECIFIED;
	}

	switch (smp->method) {
	case LEGACY_OOB:
		if (smp_auth_cb && smp_auth_cb->oob_data_request) {
			struct bt_conn_oob_info info = {
				.type = BT_CONN_OOB_LE_LEGACY,
			};

			atomic_set_bit(smp->flags, SMP_FLAG_USER);
			smp_auth_cb->oob_data_request(smp->chan.chan.conn, &info);
		} else {
			return BT_SMP_ERR_OOB_NOT_AVAIL;
		}

		break;
	case PASSKEY_DISPLAY:
		if (IS_ENABLED(CONFIG_BT_FIXED_PASSKEY) &&
		    fixed_passkey != BT_PASSKEY_INVALID) {
			passkey = fixed_passkey;
		} else  {
			if (bt_rand(&passkey, sizeof(passkey))) {
				return BT_SMP_ERR_UNSPECIFIED;
			}

			passkey %= 1000000;
		}

		if (IS_ENABLED(CONFIG_BT_LOG_SNIFFER_INFO)) {
			LOG_INF("Legacy passkey %u", passkey);
		}

		if (smp_auth_cb && smp_auth_cb->passkey_display) {
			atomic_set_bit(smp->flags, SMP_FLAG_DISPLAY);
			smp_auth_cb->passkey_display(conn, passkey);
		}

		sys_put_le32(passkey, smp->tk);

		break;
	case PASSKEY_INPUT:
		atomic_set_bit(smp->flags, SMP_FLAG_USER);
		smp_auth_cb->passkey_entry(conn);
		break;
	case JUST_WORKS:
		break;
	default:
		LOG_ERR("Unknown pairing method (%u)", smp->method);
		return BT_SMP_ERR_UNSPECIFIED;
	}

	return 0;
}

static uint8_t legacy_send_pairing_confirm(struct bt_smp *smp)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	struct bt_smp_pairing_confirm *req;
	struct net_buf *buf;

	buf = smp_create_pdu(smp, BT_SMP_CMD_PAIRING_CONFIRM, sizeof(*req));
	if (!buf) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	req = net_buf_add(buf, sizeof(*req));

	if (smp_c1(smp->tk, smp->prnd, smp->preq, smp->prsp,
		   &conn->le.init_addr, &conn->le.resp_addr, req->val)) {
		net_buf_unref(buf);
		return BT_SMP_ERR_UNSPECIFIED;
	}

	smp_send(smp, buf, NULL, NULL);

	atomic_clear_bit(smp->flags, SMP_FLAG_CFM_DELAYED);

	return 0;
}

#if defined(CONFIG_BT_PERIPHERAL)
static uint8_t legacy_pairing_req(struct bt_smp *smp)
{
	const struct bt_conn_auth_cb *smp_auth_cb = latch_auth_cb(smp);
	uint8_t ret;

	LOG_DBG("");

	ret = legacy_request_tk(smp);
	if (ret) {
		return ret;
	}

	/* ask for consent if pairing is not due to sending SecReq*/
	if ((DISPLAY_FIXED(smp) || smp->method == JUST_WORKS) &&
	    !atomic_test_bit(smp->flags, SMP_FLAG_SEC_REQ) &&
	    smp_auth_cb && smp_auth_cb->pairing_confirm) {
		atomic_set_bit(smp->flags, SMP_FLAG_USER);
		smp_auth_cb->pairing_confirm(smp->chan.chan.conn);
		return 0;
	}

	atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_CONFIRM);
	atomic_set_bit(smp->allowed_cmds, BT_SMP_KEYPRESS_NOTIFICATION);
	return send_pairing_rsp(smp);
}
#endif /* CONFIG_BT_PERIPHERAL */

static uint8_t legacy_pairing_random(struct bt_smp *smp)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	uint8_t tmp[16];
	int err;

	LOG_DBG("");

	/* calculate confirmation */
	err = smp_c1(smp->tk, smp->rrnd, smp->preq, smp->prsp,
		     &conn->le.init_addr, &conn->le.resp_addr, tmp);
	if (err) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	LOG_DBG("pcnf %s", bt_hex(smp->pcnf, 16));
	LOG_DBG("cfm %s", bt_hex(tmp, 16));

	if (memcmp(smp->pcnf, tmp, sizeof(smp->pcnf))) {
		return BT_SMP_ERR_CONFIRM_FAILED;
	}

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    conn->role == BT_HCI_ROLE_CENTRAL) {
		uint8_t ediv[2], rand[8];

		/* No need to store central STK */
		err = smp_s1(smp->tk, smp->rrnd, smp->prnd, tmp);
		if (err) {
			return BT_SMP_ERR_UNSPECIFIED;
		}

		/* Rand and EDiv are 0 for the STK */
		(void)memset(ediv, 0, sizeof(ediv));
		(void)memset(rand, 0, sizeof(rand));
		if (bt_conn_le_start_encryption(conn, rand, ediv, tmp,
						get_encryption_key_size(smp))) {
			LOG_ERR("Failed to start encryption");
			return BT_SMP_ERR_UNSPECIFIED;
		}

		atomic_set_bit(smp->flags, SMP_FLAG_ENC_PENDING);

		if (IS_ENABLED(CONFIG_BT_SMP_USB_HCI_CTLR_WORKAROUND)) {
			if (smp->remote_dist & BT_SMP_DIST_ENC_KEY) {
				atomic_set_bit(smp->allowed_cmds,
					       BT_SMP_CMD_ENCRYPT_INFO);
			} else if (smp->remote_dist & BT_SMP_DIST_ID_KEY) {
				atomic_set_bit(smp->allowed_cmds,
					       BT_SMP_CMD_IDENT_INFO);
			} else if (smp->remote_dist & BT_SMP_DIST_SIGN) {
				atomic_set_bit(smp->allowed_cmds,
					       BT_SMP_CMD_SIGNING_INFO);
			}
		}

		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		err = smp_s1(smp->tk, smp->prnd, smp->rrnd, tmp);
		if (err) {
			LOG_ERR("Calculate STK failed");
			return BT_SMP_ERR_UNSPECIFIED;
		}

		memcpy(smp->tk, tmp, sizeof(smp->tk));
		LOG_DBG("generated STK %s", bt_hex(smp->tk, 16));

		atomic_set_bit(smp->flags, SMP_FLAG_ENC_PENDING);

		return smp_send_pairing_random(smp);
	}

	return 0;
}

static uint8_t legacy_pairing_confirm(struct bt_smp *smp)
{
	LOG_DBG("");

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    smp->chan.chan.conn->role == BT_HCI_ROLE_CENTRAL) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_CONFIRM);
		return legacy_send_pairing_confirm(smp);
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		if (!atomic_test_bit(smp->flags, SMP_FLAG_USER)) {
			atomic_set_bit(smp->allowed_cmds,
				       BT_SMP_CMD_PAIRING_RANDOM);
			return legacy_send_pairing_confirm(smp);
		}

		atomic_set_bit(smp->flags, SMP_FLAG_CFM_DELAYED);
	}

	return 0;
}

static void legacy_user_tk_entry(struct bt_smp *smp)
{
	if (!atomic_test_and_clear_bit(smp->flags, SMP_FLAG_CFM_DELAYED)) {
		return;
	}

	/* if confirm failed ie. due to invalid passkey, cancel pairing */
	if (legacy_pairing_confirm(smp)) {
		smp_error(smp, BT_SMP_ERR_PASSKEY_ENTRY_FAILED);
		return;
	}

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    smp->chan.chan.conn->role == BT_HCI_ROLE_CENTRAL) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_CONFIRM);
		return;
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_RANDOM);
	}
}

static void legacy_passkey_entry(struct bt_smp *smp, unsigned int passkey)
{
	passkey = sys_cpu_to_le32(passkey);
	memcpy(smp->tk, &passkey, sizeof(passkey));

	legacy_user_tk_entry(smp);
}

static uint8_t smp_encrypt_info(struct bt_smp *smp, struct net_buf *buf)
{
	LOG_DBG("");

	if (atomic_test_bit(smp->flags, SMP_FLAG_BOND)) {
		struct bt_smp_encrypt_info *req = (void *)buf->data;
		struct bt_conn *conn = smp->chan.chan.conn;
		struct bt_keys *keys;

		keys = bt_keys_get_type(BT_KEYS_LTK, conn->id, &conn->le.dst);
		if (!keys) {
			LOG_ERR("Unable to get keys for %s", bt_addr_le_str(&conn->le.dst));
			return BT_SMP_ERR_UNSPECIFIED;
		}

		memcpy(keys->ltk.val, req->ltk, 16);
	}

	atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_CENTRAL_IDENT);

	return 0;
}

static uint8_t smp_central_ident(struct bt_smp *smp, struct net_buf *buf)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	uint8_t err;

	LOG_DBG("");

	if (atomic_test_bit(smp->flags, SMP_FLAG_BOND)) {
		struct bt_smp_central_ident *req = (void *)buf->data;
		struct bt_keys *keys;

		keys = bt_keys_get_type(BT_KEYS_LTK, conn->id, &conn->le.dst);
		if (!keys) {
			LOG_ERR("Unable to get keys for %s", bt_addr_le_str(&conn->le.dst));
			return BT_SMP_ERR_UNSPECIFIED;
		}

		memcpy(keys->ltk.ediv, req->ediv, sizeof(keys->ltk.ediv));
		memcpy(keys->ltk.rand, req->rand, sizeof(req->rand));
	}

	smp->remote_dist &= ~BT_SMP_DIST_ENC_KEY;

	if (smp->remote_dist & BT_SMP_DIST_ID_KEY) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_IDENT_INFO);
	} else if (smp->remote_dist & BT_SMP_DIST_SIGN) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_SIGNING_INFO);
	}

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    conn->role == BT_HCI_ROLE_CENTRAL && !smp->remote_dist) {
		err = bt_smp_distribute_keys(smp);
		if (err) {
			return err;
		}
	}

	/* if all keys were distributed, pairing is done */
	if (!smp->local_dist && !smp->remote_dist) {
		smp_pairing_complete(smp, 0);
	}

	return 0;
}

#if defined(CONFIG_BT_CENTRAL)
static uint8_t legacy_pairing_rsp(struct bt_smp *smp)
{
	const struct bt_conn_auth_cb *smp_auth_cb = latch_auth_cb(smp);
	uint8_t ret;

	LOG_DBG("");

	ret = legacy_request_tk(smp);
	if (ret) {
		return ret;
	}

	/* ask for consent if this is due to received SecReq */
	if ((DISPLAY_FIXED(smp) || smp->method == JUST_WORKS) &&
	    atomic_test_bit(smp->flags, SMP_FLAG_SEC_REQ) &&
	    smp_auth_cb && smp_auth_cb->pairing_confirm) {
		atomic_set_bit(smp->flags, SMP_FLAG_USER);
		smp_auth_cb->pairing_confirm(smp->chan.chan.conn);
		return 0;
	}

	if (!atomic_test_bit(smp->flags, SMP_FLAG_USER)) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_CONFIRM);
		atomic_set_bit(smp->allowed_cmds, BT_SMP_KEYPRESS_NOTIFICATION);
		return legacy_send_pairing_confirm(smp);
	}

	atomic_set_bit(smp->flags, SMP_FLAG_CFM_DELAYED);

	return 0;
}
#endif /* CONFIG_BT_CENTRAL */
#else
static uint8_t smp_encrypt_info(struct bt_smp *smp, struct net_buf *buf)
{
	return BT_SMP_ERR_CMD_NOTSUPP;
}

static uint8_t smp_central_ident(struct bt_smp *smp, struct net_buf *buf)
{
	return BT_SMP_ERR_CMD_NOTSUPP;
}
#endif /* !CONFIG_BT_SMP_SC_PAIR_ONLY */

static int smp_init(struct bt_smp *smp)
{
	/* Initialize SMP context excluding L2CAP channel context and anything
	 * else declared after.
	 */
	(void)memset(smp, 0, offsetof(struct bt_smp, chan));

	/* Generate local random number */
	if (bt_rand(smp->prnd, 16)) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	LOG_DBG("prnd %s", bt_hex(smp->prnd, 16));

	atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_FAIL);

#if !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)
	sc_public_key = bt_pub_key_get();
#endif

	return 0;
}

void bt_set_bondable(bool enable)
{
	bondable = enable;
}

void bt_le_oob_set_sc_flag(bool enable)
{
	sc_oobd_present = enable;
}

void bt_le_oob_set_legacy_flag(bool enable)
{
	legacy_oobd_present = enable;
}

static uint8_t get_auth(struct bt_smp *smp, uint8_t auth)
{
	struct bt_conn *conn = smp->chan.chan.conn;

	if (sc_supported) {
		auth &= BT_SMP_AUTH_MASK_SC;
	} else {
		auth &= BT_SMP_AUTH_MASK;
	}

	if ((get_io_capa(smp) == BT_SMP_IO_NO_INPUT_OUTPUT) ||
	    (!IS_ENABLED(CONFIG_BT_SMP_ENFORCE_MITM) &&
	    (conn->required_sec_level < BT_SECURITY_L3))) {
		auth &= ~(BT_SMP_AUTH_MITM);
	} else {
		auth |= BT_SMP_AUTH_MITM;
	}

	if (latch_bondable(smp)) {
		auth |= BT_SMP_AUTH_BONDING;
	} else {
		auth &= ~BT_SMP_AUTH_BONDING;
	}

	if (IS_ENABLED(CONFIG_BT_PASSKEY_KEYPRESS)) {
		auth |= BT_SMP_AUTH_KEYPRESS;
	} else {
		auth &= ~BT_SMP_AUTH_KEYPRESS;
	}

	return auth;
}

static uint8_t remote_sec_level_reachable(struct bt_smp *smp)
{
	bt_security_t sec = smp->chan.chan.conn->required_sec_level;

	if (IS_ENABLED(CONFIG_BT_SMP_SC_ONLY)) {
		sec = BT_SECURITY_L4;
	}
	if (IS_ENABLED(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)) {
		sec = BT_SECURITY_L3;
	}

	switch (sec) {
	case BT_SECURITY_L1:
	case BT_SECURITY_L2:
		return 0;

	case BT_SECURITY_L4:
		if (get_encryption_key_size(smp) != BT_SMP_MAX_ENC_KEY_SIZE) {
			return BT_SMP_ERR_ENC_KEY_SIZE;
		}

		if (!atomic_test_bit(smp->flags, SMP_FLAG_SC)) {
			return BT_SMP_ERR_AUTH_REQUIREMENTS;
		}
		__fallthrough;
	case BT_SECURITY_L3:
		if (smp->method == JUST_WORKS) {
			return BT_SMP_ERR_AUTH_REQUIREMENTS;
		}

		return 0;
	default:
		return BT_SMP_ERR_UNSPECIFIED;
	}
}

static bool sec_level_reachable(struct bt_smp *smp)
{
	const struct bt_conn_auth_cb *smp_auth_cb = latch_auth_cb(smp);

	switch (smp->chan.chan.conn->required_sec_level) {
	case BT_SECURITY_L1:
	case BT_SECURITY_L2:
		return true;
	case BT_SECURITY_L3:
		return get_io_capa(smp) != BT_SMP_IO_NO_INPUT_OUTPUT ||
		       (smp_auth_cb && smp_auth_cb->oob_data_request);
	case BT_SECURITY_L4:
		return (get_io_capa(smp) != BT_SMP_IO_NO_INPUT_OUTPUT ||
		       (smp_auth_cb && smp_auth_cb->oob_data_request)) && sc_supported;
	default:
		return false;
	}
}

static struct bt_smp *smp_chan_get(struct bt_conn *conn)
{
	struct bt_l2cap_chan *chan;

	chan = bt_l2cap_le_lookup_rx_cid(conn, BT_L2CAP_CID_SMP);
	if (!chan) {
		LOG_ERR("Unable to find SMP channel");
		return NULL;
	}

	return CONTAINER_OF(chan, struct bt_smp, chan.chan);
}

bool bt_smp_request_ltk(struct bt_conn *conn, uint64_t rand, uint16_t ediv, uint8_t *ltk)
{
	struct bt_smp *smp;
	uint8_t enc_size;

	smp = smp_chan_get(conn);
	if (!smp) {
		return false;
	}

	/*
	 * Both legacy STK and LE SC LTK have rand and ediv equal to zero.
	 * If pairing is in progress use the TK for encryption.
	 */
	if (ediv == 0U && rand == 0U &&
	    atomic_test_bit(smp->flags, SMP_FLAG_PAIRING) &&
	    atomic_test_bit(smp->flags, SMP_FLAG_ENC_PENDING)) {
		enc_size = get_encryption_key_size(smp);

		/*
		 * We keep both legacy STK and LE SC LTK in TK.
		 * Also use only enc_size bytes of key for encryption.
		 */
		memcpy(ltk, smp->tk, enc_size);
		if (enc_size < BT_SMP_MAX_ENC_KEY_SIZE) {
			(void)memset(ltk + enc_size, 0,
				     BT_SMP_MAX_ENC_KEY_SIZE - enc_size);
		}

		atomic_set_bit(smp->flags, SMP_FLAG_ENC_PENDING);
		return true;
	}

	if (!conn->le.keys) {
		conn->le.keys = bt_keys_find(BT_KEYS_LTK_P256, conn->id,
					     &conn->le.dst);
		if (!conn->le.keys) {
			conn->le.keys = bt_keys_find(BT_KEYS_PERIPH_LTK,
						     conn->id, &conn->le.dst);
		}
	}

	if (ediv == 0U && rand == 0U &&
	    conn->le.keys && (conn->le.keys->keys & BT_KEYS_LTK_P256)) {
		enc_size = conn->le.keys->enc_size;

		memcpy(ltk, conn->le.keys->ltk.val, enc_size);
		if (enc_size < BT_SMP_MAX_ENC_KEY_SIZE) {
			(void)memset(ltk + enc_size, 0,
				     BT_SMP_MAX_ENC_KEY_SIZE - enc_size);
		}

		atomic_set_bit(smp->flags, SMP_FLAG_ENC_PENDING);
		return true;
	}

#if !defined(CONFIG_BT_SMP_SC_PAIR_ONLY)
	if (conn->le.keys && (conn->le.keys->keys & BT_KEYS_PERIPH_LTK) &&
	    !memcmp(conn->le.keys->periph_ltk.rand, &rand, 8) &&
	    !memcmp(conn->le.keys->periph_ltk.ediv, &ediv, 2)) {
		enc_size = conn->le.keys->enc_size;

		memcpy(ltk, conn->le.keys->periph_ltk.val, enc_size);
		if (enc_size < BT_SMP_MAX_ENC_KEY_SIZE) {
			(void)memset(ltk + enc_size, 0,
				     BT_SMP_MAX_ENC_KEY_SIZE - enc_size);
		}

		atomic_set_bit(smp->flags, SMP_FLAG_ENC_PENDING);
		return true;
	}
#endif /* !CONFIG_BT_SMP_SC_PAIR_ONLY */

	if (atomic_test_bit(smp->flags, SMP_FLAG_SEC_REQ)) {
		/* Notify higher level that security failed if security was
		 * initiated by peripheral.
		 */
		bt_conn_security_changed(conn, BT_HCI_ERR_PIN_OR_KEY_MISSING,
					 BT_SECURITY_ERR_PIN_OR_KEY_MISSING);
	}

	smp_reset(smp);
	return false;
}

#if defined(CONFIG_BT_PERIPHERAL)
static int smp_send_security_req(struct bt_conn *conn)
{
	struct bt_smp *smp;
	struct bt_smp_security_request *req;
	struct net_buf *req_buf;
	int err;

	LOG_DBG("");
	smp = smp_chan_get(conn);
	if (!smp) {
		return -ENOTCONN;
	}

	/* SMP Timeout */
	if (atomic_test_bit(smp->flags, SMP_FLAG_TIMEOUT)) {
		return -EIO;
	}

	/* pairing is in progress */
	if (atomic_test_bit(smp->flags, SMP_FLAG_PAIRING)) {
		return -EBUSY;
	}

	if (atomic_test_bit(smp->flags, SMP_FLAG_ENC_PENDING)) {
		return -EBUSY;
	}

	/* early verify if required sec level if reachable */
	if (!(sec_level_reachable(smp) || smp_keys_check(conn))) {
		return -EINVAL;
	}

	if (!conn->le.keys) {
		conn->le.keys = bt_keys_get_addr(conn->id, &conn->le.dst);
		if (!conn->le.keys) {
			return -ENOMEM;
		}
	}

	if (smp_init(smp) != 0) {
		return -ENOBUFS;
	}

	req_buf = smp_create_pdu(smp, BT_SMP_CMD_SECURITY_REQUEST,
				 sizeof(*req));
	if (!req_buf) {
		return -ENOBUFS;
	}

	req = net_buf_add(req_buf, sizeof(*req));
	req->auth_req = get_auth(smp, BT_SMP_AUTH_DEFAULT);

	/* SMP timer is not restarted for SecRequest so don't use smp_send */
	err = bt_l2cap_send_pdu(&smp->chan, req_buf, NULL, NULL);
	if (err) {
		net_buf_unref(req_buf);
		return err;
	}

	atomic_set_bit(smp->flags, SMP_FLAG_SEC_REQ);
	atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_REQ);

	return 0;
}

static uint8_t smp_pairing_req(struct bt_smp *smp, struct net_buf *buf)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	const struct bt_conn_auth_cb *smp_auth_cb = latch_auth_cb(smp);

	struct bt_smp_pairing *req = (void *)buf->data;
	struct bt_smp_pairing *rsp;
	uint8_t err;

	LOG_DBG("req: io_capability 0x%02X, oob_flag 0x%02X, auth_req 0x%02X, "
		"max_key_size 0x%02X, init_key_dist 0x%02X, resp_key_dist 0x%02X",
		req->io_capability, req->oob_flag, req->auth_req,
		req->max_key_size, req->init_key_dist, req->resp_key_dist);

	if ((req->max_key_size > BT_SMP_MAX_ENC_KEY_SIZE) ||
	    (req->max_key_size < BT_SMP_MIN_ENC_KEY_SIZE)) {
		return BT_SMP_ERR_ENC_KEY_SIZE;
	}

	if (!conn->le.keys) {
		conn->le.keys = bt_keys_get_addr(conn->id, &conn->le.dst);
		if (!conn->le.keys) {
			LOG_DBG("Unable to get keys for %s", bt_addr_le_str(&conn->le.dst));
			return BT_SMP_ERR_UNSPECIFIED;
		}
	}

	/* If we already sent a security request then the SMP context
	 * is already initialized.
	 */
	if (!atomic_test_bit(smp->flags, SMP_FLAG_SEC_REQ)) {
		int ret = smp_init(smp);

		if (ret) {
			return ret;
		}
	}

	/* Store req for later use */
	smp->preq[0] = BT_SMP_CMD_PAIRING_REQ;
	memcpy(smp->preq + 1, req, sizeof(*req));

	/* create rsp, it will be used later on */
	smp->prsp[0] = BT_SMP_CMD_PAIRING_RSP;
	rsp = (struct bt_smp_pairing *)&smp->prsp[1];

	rsp->auth_req = get_auth(smp, req->auth_req);
	rsp->io_capability = get_io_capa(smp);
	rsp->max_key_size = BT_SMP_MAX_ENC_KEY_SIZE;
	rsp->init_key_dist = (req->init_key_dist & RECV_KEYS);
	rsp->resp_key_dist = (req->resp_key_dist & SEND_KEYS);

	if ((rsp->auth_req & BT_SMP_AUTH_SC) &&
	    (req->auth_req & BT_SMP_AUTH_SC)) {
		atomic_set_bit(smp->flags, SMP_FLAG_SC);

		rsp->init_key_dist &= RECV_KEYS_SC;
		rsp->resp_key_dist &= SEND_KEYS_SC;
	}

	if (atomic_test_bit(smp->flags, SMP_FLAG_SC)) {
		rsp->oob_flag = sc_oobd_present ? BT_SMP_OOB_PRESENT :
				BT_SMP_OOB_NOT_PRESENT;
	} else {
		rsp->oob_flag = legacy_oobd_present ? BT_SMP_OOB_PRESENT :
				BT_SMP_OOB_NOT_PRESENT;
	}

	if ((rsp->auth_req & BT_SMP_AUTH_CT2) &&
	    (req->auth_req & BT_SMP_AUTH_CT2)) {
		atomic_set_bit(smp->flags, SMP_FLAG_CT2);
	}

	if ((rsp->auth_req & BT_SMP_AUTH_BONDING) &&
	    (req->auth_req & BT_SMP_AUTH_BONDING)) {
		atomic_set_bit(smp->flags, SMP_FLAG_BOND);
	} else if (IS_ENABLED(CONFIG_BT_BONDING_REQUIRED)) {
		/* Reject pairing req if not both intend to bond */
		LOG_DBG("Bonding required");
		return BT_SMP_ERR_UNSPECIFIED;
	} else {
		rsp->init_key_dist = 0;
		rsp->resp_key_dist = 0;
	}

	smp->local_dist = rsp->resp_key_dist;
	smp->remote_dist = rsp->init_key_dist;

	atomic_set_bit(smp->flags, SMP_FLAG_PAIRING);

	smp->method = get_pair_method(smp, req->io_capability);

	if (!update_keys_check(smp, conn->le.keys)) {
		return BT_SMP_ERR_AUTH_REQUIREMENTS;
	}

	err = remote_sec_level_reachable(smp);
	if (err) {
		return err;
	}

	if (!atomic_test_bit(smp->flags, SMP_FLAG_SC)) {
#if defined(CONFIG_BT_SMP_SC_PAIR_ONLY)
		return BT_SMP_ERR_AUTH_REQUIREMENTS;
#else
		if (IS_ENABLED(CONFIG_BT_SMP_APP_PAIRING_ACCEPT)) {
			err = smp_pairing_accept_query(smp, req);
			if (err) {
				return err;
			}
		}

		return legacy_pairing_req(smp);
#endif /* CONFIG_BT_SMP_SC_PAIR_ONLY */
	}

	if (IS_ENABLED(CONFIG_BT_SMP_APP_PAIRING_ACCEPT)) {
		err = smp_pairing_accept_query(smp, req);
		if (err) {
			return err;
		}
	}

	if (!IS_ENABLED(CONFIG_BT_SMP_SC_PAIR_ONLY) &&
	    (DISPLAY_FIXED(smp) || smp->method == JUST_WORKS) &&
	    !atomic_test_bit(smp->flags, SMP_FLAG_SEC_REQ) &&
	    smp_auth_cb && smp_auth_cb->pairing_confirm) {
		atomic_set_bit(smp->flags, SMP_FLAG_USER);
		smp_auth_cb->pairing_confirm(conn);
		return 0;
	}

	atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PUBLIC_KEY);

	LOG_DBG("rsp: io_capability 0x%02X, oob_flag 0x%02X, auth_req 0x%02X, "
		"max_key_size 0x%02X, init_key_dist 0x%02X, resp_key_dist 0x%02X",
		rsp->io_capability, rsp->oob_flag, rsp->auth_req,
		rsp->max_key_size, rsp->init_key_dist, rsp->resp_key_dist);

	return send_pairing_rsp(smp);
}
#else
static uint8_t smp_pairing_req(struct bt_smp *smp, struct net_buf *buf)
{
	return BT_SMP_ERR_CMD_NOTSUPP;
}
#endif /* CONFIG_BT_PERIPHERAL */

static uint8_t sc_send_public_key(struct bt_smp *smp)
{
	struct bt_smp_public_key *req;
	struct net_buf *req_buf;

	req_buf = smp_create_pdu(smp, BT_SMP_CMD_PUBLIC_KEY, sizeof(*req));
	if (!req_buf) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	req = net_buf_add(req_buf, sizeof(*req));

	memcpy(req->x, sc_public_key, sizeof(req->x));
	memcpy(req->y, &sc_public_key[32], sizeof(req->y));

	smp_send(smp, req_buf, NULL, NULL);

	if (IS_ENABLED(CONFIG_BT_USE_DEBUG_KEYS)) {
		atomic_set_bit(smp->flags, SMP_FLAG_SC_DEBUG_KEY);
	}

	return 0;
}

#if defined(CONFIG_BT_CENTRAL)
static int smp_send_pairing_req(struct bt_conn *conn)
{
	struct bt_smp *smp;
	struct bt_smp_pairing *req;
	struct net_buf *req_buf;

	LOG_DBG("");

	smp = smp_chan_get(conn);
	if (!smp) {
		return -ENOTCONN;
	}

	/* SMP Timeout */
	if (atomic_test_bit(smp->flags, SMP_FLAG_TIMEOUT)) {
		return -EIO;
	}

	/* A higher security level is requested during the key distribution
	 * phase, once pairing is complete a new pairing procedure will start.
	 */
	if (atomic_test_bit(smp->flags, SMP_FLAG_KEYS_DISTR)) {
		return 0;
	}

	/* pairing is in progress */
	if (atomic_test_bit(smp->flags, SMP_FLAG_PAIRING)) {
		return -EBUSY;
	}

	/* Encryption is in progress */
	if (atomic_test_bit(smp->flags, SMP_FLAG_ENC_PENDING)) {
		return -EBUSY;
	}

	/* early verify if required sec level if reachable */
	if (!sec_level_reachable(smp)) {
		return -EINVAL;
	}

	if (!conn->le.keys) {
		conn->le.keys = bt_keys_get_addr(conn->id, &conn->le.dst);
		if (!conn->le.keys) {
			return -ENOMEM;
		}
	}

	if (smp_init(smp)) {
		return -ENOBUFS;
	}

	req_buf = smp_create_pdu(smp, BT_SMP_CMD_PAIRING_REQ, sizeof(*req));
	if (!req_buf) {
		return -ENOBUFS;
	}

	req = net_buf_add(req_buf, sizeof(*req));

	req->auth_req = get_auth(smp, BT_SMP_AUTH_DEFAULT);
	req->io_capability = get_io_capa(smp);

	/* At this point is it unknown if pairing will be legacy or LE SC so
	 * set OOB flag if any OOB data is present and assume to peer device
	 * provides OOB data that will match it's pairing type.
	 */
	req->oob_flag = (legacy_oobd_present || sc_oobd_present) ?
				BT_SMP_OOB_PRESENT : BT_SMP_OOB_NOT_PRESENT;

	req->max_key_size = BT_SMP_MAX_ENC_KEY_SIZE;

	if (req->auth_req & BT_SMP_AUTH_BONDING) {
		req->init_key_dist = SEND_KEYS;
		req->resp_key_dist = RECV_KEYS;
	} else {
		req->init_key_dist = 0;
		req->resp_key_dist = 0;
	}

	smp->local_dist = req->init_key_dist;
	smp->remote_dist = req->resp_key_dist;

	/* Store req for later use */
	smp->preq[0] = BT_SMP_CMD_PAIRING_REQ;
	memcpy(smp->preq + 1, req, sizeof(*req));

	LOG_DBG("req: io_capability 0x%02X, oob_flag 0x%02X, auth_req 0x%02X, "
		"max_key_size 0x%02X, init_key_dist 0x%02X, resp_key_dist 0x%02X",
		req->io_capability, req->oob_flag, req->auth_req,
		req->max_key_size, req->init_key_dist, req->resp_key_dist);

	smp_send(smp, req_buf, NULL, NULL);

	atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_RSP);
	atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_SECURITY_REQUEST);
	atomic_set_bit(smp->flags, SMP_FLAG_PAIRING);

	return 0;
}

static uint8_t smp_pairing_rsp(struct bt_smp *smp, struct net_buf *buf)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	struct bt_smp_pairing *rsp = (void *)buf->data;
	struct bt_smp_pairing *req = (struct bt_smp_pairing *)&smp->preq[1];
	uint8_t err;

	LOG_DBG("rsp: io_capability 0x%02X, oob_flag 0x%02X, auth_req 0x%02X, "
		"max_key_size 0x%02X, init_key_dist 0x%02X, resp_key_dist 0x%02X",
		rsp->io_capability, rsp->oob_flag, rsp->auth_req,
		rsp->max_key_size, rsp->init_key_dist, rsp->resp_key_dist);

	if ((rsp->max_key_size > BT_SMP_MAX_ENC_KEY_SIZE) ||
	    (rsp->max_key_size < BT_SMP_MIN_ENC_KEY_SIZE)) {
		return BT_SMP_ERR_ENC_KEY_SIZE;
	}

	smp->local_dist &= rsp->init_key_dist;
	smp->remote_dist &= rsp->resp_key_dist;

	/* Store rsp for later use */
	smp->prsp[0] = BT_SMP_CMD_PAIRING_RSP;
	memcpy(smp->prsp + 1, rsp, sizeof(*rsp));

	if ((rsp->auth_req & BT_SMP_AUTH_SC) &&
	    (req->auth_req & BT_SMP_AUTH_SC)) {
		atomic_set_bit(smp->flags, SMP_FLAG_SC);
	}

	if ((rsp->auth_req & BT_SMP_AUTH_CT2) &&
	    (req->auth_req & BT_SMP_AUTH_CT2)) {
		atomic_set_bit(smp->flags, SMP_FLAG_CT2);
	}

	if ((rsp->auth_req & BT_SMP_AUTH_BONDING) &&
	    (req->auth_req & BT_SMP_AUTH_BONDING)) {
		atomic_set_bit(smp->flags, SMP_FLAG_BOND);
	} else if (IS_ENABLED(CONFIG_BT_BONDING_REQUIRED)) {
		/* Reject pairing req if not both intend to bond */
		LOG_DBG("Bonding required");
		return BT_SMP_ERR_UNSPECIFIED;
	} else {
		smp->local_dist = 0;
		smp->remote_dist = 0;
	}

	smp->method = get_pair_method(smp, rsp->io_capability);

	if (!update_keys_check(smp, conn->le.keys)) {
		return BT_SMP_ERR_AUTH_REQUIREMENTS;
	}

	err = remote_sec_level_reachable(smp);
	if (err) {
		return err;
	}

	/* the OR operation evaluated by "if" statement bellow seems redundant
	 * when CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY is enabled, because in
	 * that case the SMP_FLAG_SC will always be set to false. But it's
	 * needed in order to inform the compiler that the inside of the "if"
	 * is the return point for CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY enabled
	 * builds. This avoids compiler warnings regarding the code after the
	 * "if" statement, that would happen for builds with
	 * CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY enabled
	 */
	if (IS_ENABLED(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY) ||
	    !atomic_test_bit(smp->flags, SMP_FLAG_SC)) {
#if defined(CONFIG_BT_SMP_SC_PAIR_ONLY)
		return BT_SMP_ERR_AUTH_REQUIREMENTS;
#else
		if (IS_ENABLED(CONFIG_BT_SMP_APP_PAIRING_ACCEPT)) {
			err = smp_pairing_accept_query(smp, rsp);
			if (err) {
				return err;
			}
		}

		return legacy_pairing_rsp(smp);
#endif /* CONFIG_BT_SMP_SC_PAIR_ONLY */
	}

	const struct bt_conn_auth_cb *smp_auth_cb = latch_auth_cb(smp);

	smp->local_dist &= SEND_KEYS_SC;
	smp->remote_dist &= RECV_KEYS_SC;

	if (IS_ENABLED(CONFIG_BT_SMP_APP_PAIRING_ACCEPT)) {
		err = smp_pairing_accept_query(smp, rsp);
		if (err) {
			return err;
		}
	}

	if (!IS_ENABLED(CONFIG_BT_SMP_SC_PAIR_ONLY) &&
	    (DISPLAY_FIXED(smp) || smp->method == JUST_WORKS) &&
	    atomic_test_bit(smp->flags, SMP_FLAG_SEC_REQ) &&
	    smp_auth_cb && smp_auth_cb->pairing_confirm) {
		atomic_set_bit(smp->flags, SMP_FLAG_USER);
		smp_auth_cb->pairing_confirm(conn);
		return 0;
	}

	if (!sc_public_key) {
		atomic_set_bit(smp->flags, SMP_FLAG_PKEY_SEND);
		return 0;
	}

	atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PUBLIC_KEY);
	atomic_clear_bit(smp->allowed_cmds, BT_SMP_CMD_SECURITY_REQUEST);

	return sc_send_public_key(smp);
}
#else
static uint8_t smp_pairing_rsp(struct bt_smp *smp, struct net_buf *buf)
{
	return BT_SMP_ERR_CMD_NOTSUPP;
}
#endif /* CONFIG_BT_CENTRAL */

static uint8_t smp_pairing_confirm(struct bt_smp *smp, struct net_buf *buf)
{
	struct bt_smp_pairing_confirm *req = (void *)buf->data;

	LOG_DBG("");

	atomic_clear_bit(smp->flags, SMP_FLAG_DISPLAY);

	memcpy(smp->pcnf, req->val, sizeof(smp->pcnf));

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    smp->chan.chan.conn->role == BT_HCI_ROLE_CENTRAL) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_RANDOM);
		return smp_send_pairing_random(smp);
	}

	if (!IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		return 0;
	}

#if !defined(CONFIG_BT_SMP_SC_PAIR_ONLY)
	if (!atomic_test_bit(smp->flags, SMP_FLAG_SC)) {
		return legacy_pairing_confirm(smp);
	}
#endif /* !CONFIG_BT_SMP_SC_PAIR_ONLY */

	switch (smp->method) {
	case PASSKEY_DISPLAY:
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_RANDOM);
		return smp_send_pairing_confirm(smp);
	case PASSKEY_INPUT:
		if (atomic_test_bit(smp->flags, SMP_FLAG_USER)) {
			atomic_set_bit(smp->flags, SMP_FLAG_CFM_DELAYED);
			return 0;
		}

		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_RANDOM);
		return smp_send_pairing_confirm(smp);
	case JUST_WORKS:
	case PASSKEY_CONFIRM:
	default:
		LOG_ERR("Unknown pairing method (%u)", smp->method);
		return BT_SMP_ERR_UNSPECIFIED;
	}
}

static uint8_t sc_smp_send_dhkey_check(struct bt_smp *smp, const uint8_t *e)
{
	struct bt_smp_dhkey_check *req;
	struct net_buf *buf;

	LOG_DBG("");

	buf = smp_create_pdu(smp, BT_SMP_DHKEY_CHECK, sizeof(*req));
	if (!buf) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	req = net_buf_add(buf, sizeof(*req));
	memcpy(req->e, e, sizeof(req->e));

	smp_send(smp, buf, NULL, NULL);

	return 0;
}

#if defined(CONFIG_BT_CENTRAL)
static uint8_t compute_and_send_central_dhcheck(struct bt_smp *smp)
{
	uint8_t e[16], r[16];

	(void)memset(r, 0, sizeof(r));

	switch (smp->method) {
	case JUST_WORKS:
	case PASSKEY_CONFIRM:
		break;
	case PASSKEY_DISPLAY:
	case PASSKEY_INPUT:
		memcpy(r, &smp->passkey, sizeof(smp->passkey));
		break;
	case LE_SC_OOB:
		if (smp->oobd_remote) {
			memcpy(r, smp->oobd_remote->r, sizeof(r));
		}
		break;
	default:
		LOG_ERR("Unknown pairing method (%u)", smp->method);
		return BT_SMP_ERR_UNSPECIFIED;
	}

	/* calculate LTK and mackey */
	if (bt_crypto_f5(smp->dhkey, smp->prnd, smp->rrnd, &smp->chan.chan.conn->le.init_addr,
			 &smp->chan.chan.conn->le.resp_addr, smp->mackey, smp->tk)) {
		LOG_ERR("Calculate LTK failed");
		return BT_SMP_ERR_UNSPECIFIED;
	}
	/* calculate local DHKey check */
	if (bt_crypto_f6(smp->mackey, smp->prnd, smp->rrnd, r, &smp->preq[1],
			 &smp->chan.chan.conn->le.init_addr, &smp->chan.chan.conn->le.resp_addr,
			 e)) {
		LOG_ERR("Calculate local DHKey check failed");
		return BT_SMP_ERR_UNSPECIFIED;
	}

	atomic_set_bit(smp->allowed_cmds, BT_SMP_DHKEY_CHECK);
	return sc_smp_send_dhkey_check(smp, e);
}
#endif /* CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_PERIPHERAL)
static uint8_t compute_and_check_and_send_periph_dhcheck(struct bt_smp *smp)
{
	uint8_t re[16], e[16], r[16];
	uint8_t err;

	(void)memset(r, 0, sizeof(r));

	switch (smp->method) {
	case JUST_WORKS:
	case PASSKEY_CONFIRM:
		break;
	case PASSKEY_DISPLAY:
	case PASSKEY_INPUT:
		memcpy(r, &smp->passkey, sizeof(smp->passkey));
		break;
	case LE_SC_OOB:
		if (smp->oobd_remote) {
			memcpy(r, smp->oobd_remote->r, sizeof(r));
		}
		break;
	default:
		LOG_ERR("Unknown pairing method (%u)", smp->method);
		return BT_SMP_ERR_UNSPECIFIED;
	}

	/* calculate LTK and mackey */
	if (bt_crypto_f5(smp->dhkey, smp->rrnd, smp->prnd, &smp->chan.chan.conn->le.init_addr,
			 &smp->chan.chan.conn->le.resp_addr, smp->mackey, smp->tk)) {
		LOG_ERR("Calculate LTK failed");
		return BT_SMP_ERR_UNSPECIFIED;
	}

	/* calculate local DHKey check */
	if (bt_crypto_f6(smp->mackey, smp->prnd, smp->rrnd, r, &smp->prsp[1],
			 &smp->chan.chan.conn->le.resp_addr, &smp->chan.chan.conn->le.init_addr,
			 e)) {
		LOG_ERR("Calculate local DHKey check failed");
		return BT_SMP_ERR_UNSPECIFIED;
	}

	if (smp->method == LE_SC_OOB) {
		if (smp->oobd_local) {
			memcpy(r, smp->oobd_local->r, sizeof(r));
		} else {
			memset(r, 0, sizeof(r));
		}
	}

	/* calculate remote DHKey check */
	if (bt_crypto_f6(smp->mackey, smp->rrnd, smp->prnd, r, &smp->preq[1],
			 &smp->chan.chan.conn->le.init_addr, &smp->chan.chan.conn->le.resp_addr,
			 re)) {
		LOG_ERR("Calculate remote DHKey check failed");
		return BT_SMP_ERR_UNSPECIFIED;
	}

	/* compare received E with calculated remote */
	if (memcmp(smp->e, re, 16)) {
		return BT_SMP_ERR_DHKEY_CHECK_FAILED;
	}

	/* send local e */
	err = sc_smp_send_dhkey_check(smp, e);
	if (err) {
		return err;
	}

	atomic_set_bit(smp->flags, SMP_FLAG_ENC_PENDING);
	return 0;
}
#endif /* CONFIG_BT_PERIPHERAL */

static void bt_smp_dhkey_ready(const uint8_t *dhkey);
static uint8_t smp_dhkey_generate(struct bt_smp *smp)
{
	int err;

	atomic_set_bit(smp->flags, SMP_FLAG_DHKEY_GEN);
	err = bt_dh_key_gen(smp->pkey, bt_smp_dhkey_ready);
	if (err) {
		atomic_clear_bit(smp->flags, SMP_FLAG_DHKEY_GEN);

		LOG_ERR("Failed to generate DHKey");
		return BT_SMP_ERR_UNSPECIFIED;
	}

	return 0;
}

static uint8_t smp_dhkey_ready(struct bt_smp *smp, const uint8_t *dhkey)
{
	if (!dhkey) {
		return BT_SMP_ERR_DHKEY_CHECK_FAILED;
	}

	atomic_clear_bit(smp->flags, SMP_FLAG_DHKEY_PENDING);
	memcpy(smp->dhkey, dhkey, BT_DH_KEY_LEN);

	/* wait for user passkey confirmation */
	if (atomic_test_bit(smp->flags, SMP_FLAG_USER)) {
		atomic_set_bit(smp->flags, SMP_FLAG_DHKEY_SEND);
		return 0;
	}

	/* wait for remote DHKey Check */
	if (atomic_test_bit(smp->flags, SMP_FLAG_DHCHECK_WAIT)) {
		atomic_set_bit(smp->flags, SMP_FLAG_DHKEY_SEND);
		return 0;
	}

	if (atomic_test_bit(smp->flags, SMP_FLAG_DHKEY_SEND)) {
#if defined(CONFIG_BT_CENTRAL)
		if (smp->chan.chan.conn->role == BT_HCI_ROLE_CENTRAL) {
			return compute_and_send_central_dhcheck(smp);
		}

#endif /* CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_PERIPHERAL)
		return  compute_and_check_and_send_periph_dhcheck(smp);
#endif /* CONFIG_BT_PERIPHERAL */
	}

	return 0;
}

static struct bt_smp *smp_find(int flag)
{
	for (int i = 0; i < ARRAY_SIZE(bt_smp_pool); i++) {
		if (atomic_test_bit(bt_smp_pool[i].flags, flag)) {
			return &bt_smp_pool[i];
		}
	}

	return NULL;
}

static void bt_smp_dhkey_ready(const uint8_t *dhkey)
{
	LOG_DBG("%p", (void *)dhkey);
	int err;

	struct bt_smp *smp = smp_find(SMP_FLAG_DHKEY_GEN);
	if (smp) {
		atomic_clear_bit(smp->flags, SMP_FLAG_DHKEY_GEN);
		err = smp_dhkey_ready(smp, dhkey);
		if (err) {
			smp_error(smp, err);
		}
	}

	err = 0;
	do {
		smp = smp_find(SMP_FLAG_DHKEY_PENDING);
		if (smp) {
			err = smp_dhkey_generate(smp);
			if (err) {
				smp_error(smp, err);
			}
		}
	} while (smp && err);
}

static uint8_t sc_smp_check_confirm(struct bt_smp *smp)
{
	uint8_t cfm[16];
	uint8_t r;

	switch (smp->method) {
	case LE_SC_OOB:
		return 0;
	case PASSKEY_CONFIRM:
	case JUST_WORKS:
		r = 0U;
		break;
	case PASSKEY_DISPLAY:
	case PASSKEY_INPUT:
		/*
		 * In the Passkey Entry protocol, the most significant
		 * bit of Z is set equal to one and the least
		 * significant bit is made up from one bit of the
		 * passkey e.g. if the passkey bit is 1, then Z = 0x81
		 * and if the passkey bit is 0, then Z = 0x80.
		 */
		r = (smp->passkey >> smp->passkey_round) & 0x01;
		r |= 0x80;
		break;
	default:
		LOG_ERR("Unknown pairing method (%u)", smp->method);
		return BT_SMP_ERR_UNSPECIFIED;
	}

	if (bt_crypto_f4(smp->pkey, sc_public_key, smp->rrnd, r, cfm)) {
		LOG_ERR("Calculate confirm failed");
		return BT_SMP_ERR_UNSPECIFIED;
	}

	LOG_DBG("pcnf %s", bt_hex(smp->pcnf, 16));
	LOG_DBG("cfm %s", bt_hex(cfm, 16));

	if (memcmp(smp->pcnf, cfm, 16)) {
		return BT_SMP_ERR_CONFIRM_FAILED;
	}

	return 0;
}

#ifndef CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY
static bool le_sc_oob_data_req_check(struct bt_smp *smp)
{
	struct bt_smp_pairing *req = (struct bt_smp_pairing *)&smp->preq[1];

	return ((req->oob_flag & BT_SMP_OOB_DATA_MASK) == BT_SMP_OOB_PRESENT);
}

static bool le_sc_oob_data_rsp_check(struct bt_smp *smp)
{
	struct bt_smp_pairing *rsp = (struct bt_smp_pairing *)&smp->prsp[1];

	return ((rsp->oob_flag & BT_SMP_OOB_DATA_MASK) == BT_SMP_OOB_PRESENT);
}

static void le_sc_oob_config_set(struct bt_smp *smp,
				 struct bt_conn_oob_info *info)
{
	bool req_oob_present = le_sc_oob_data_req_check(smp);
	bool rsp_oob_present = le_sc_oob_data_rsp_check(smp);
	int oob_config = BT_CONN_OOB_NO_DATA;

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    smp->chan.chan.conn->role == BT_HCI_ROLE_CENTRAL) {
		oob_config = req_oob_present ? BT_CONN_OOB_REMOTE_ONLY :
					       BT_CONN_OOB_NO_DATA;

		if (rsp_oob_present) {
			oob_config = (oob_config == BT_CONN_OOB_REMOTE_ONLY) ?
				     BT_CONN_OOB_BOTH_PEERS :
				     BT_CONN_OOB_LOCAL_ONLY;
		}
	} else if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		oob_config = req_oob_present ? BT_CONN_OOB_LOCAL_ONLY :
					       BT_CONN_OOB_NO_DATA;

		if (rsp_oob_present) {
			oob_config = (oob_config == BT_CONN_OOB_LOCAL_ONLY) ?
				     BT_CONN_OOB_BOTH_PEERS :
				     BT_CONN_OOB_REMOTE_ONLY;
		}
	}

	info->lesc.oob_config = oob_config;
}
#endif /* CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY */

static uint8_t smp_pairing_random(struct bt_smp *smp, struct net_buf *buf)
{
	const struct bt_conn_auth_cb *smp_auth_cb = latch_auth_cb(smp);
	struct bt_smp_pairing_random *req = (void *)buf->data;
	uint32_t passkey;
	uint8_t err;

	LOG_DBG("");

	memcpy(smp->rrnd, req->val, sizeof(smp->rrnd));

#if !defined(CONFIG_BT_SMP_SC_PAIR_ONLY)
	if (!atomic_test_bit(smp->flags, SMP_FLAG_SC)) {
		return legacy_pairing_random(smp);
	}
#endif /* !CONFIG_BT_SMP_SC_PAIR_ONLY */

#if defined(CONFIG_BT_CENTRAL)
	if (smp->chan.chan.conn->role == BT_HCI_ROLE_CENTRAL) {
		err = sc_smp_check_confirm(smp);
		if (err) {
			return err;
		}

		switch (smp->method) {
		case PASSKEY_CONFIRM:
			/* compare passkey before calculating LTK */
			if (bt_crypto_g2(sc_public_key, smp->pkey, smp->prnd, smp->rrnd,
					 &passkey)) {
				return BT_SMP_ERR_UNSPECIFIED;
			}

			atomic_set_bit(smp->flags, SMP_FLAG_USER);
			atomic_set_bit(smp->flags, SMP_FLAG_DHKEY_SEND);
			smp_auth_cb->passkey_confirm(smp->chan.chan.conn, passkey);
			return 0;
		case JUST_WORKS:
			break;
		case LE_SC_OOB:
			break;
		case PASSKEY_DISPLAY:
		case PASSKEY_INPUT:
			smp->passkey_round++;
			if (smp->passkey_round == 20U) {
				break;
			}

			if (bt_rand(smp->prnd, 16)) {
				return BT_SMP_ERR_UNSPECIFIED;
			}

			atomic_set_bit(smp->allowed_cmds,
				       BT_SMP_CMD_PAIRING_CONFIRM);
			return smp_send_pairing_confirm(smp);
		default:
			LOG_ERR("Unknown pairing method (%u)", smp->method);
			return BT_SMP_ERR_UNSPECIFIED;
		}

		/* wait for DHKey being generated */
		if (atomic_test_bit(smp->flags, SMP_FLAG_DHKEY_PENDING)) {
			atomic_set_bit(smp->flags, SMP_FLAG_DHKEY_SEND);
			return 0;
		}

		return compute_and_send_central_dhcheck(smp);
	}
#endif /* CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_PERIPHERAL)
	switch (smp->method) {
	case PASSKEY_CONFIRM:
		if (bt_crypto_g2(smp->pkey, sc_public_key, smp->rrnd, smp->prnd, &passkey)) {
			return BT_SMP_ERR_UNSPECIFIED;
		}

		atomic_set_bit(smp->flags, SMP_FLAG_USER);
		smp_auth_cb->passkey_confirm(smp->chan.chan.conn, passkey);
		break;
	case JUST_WORKS:
		break;
	case PASSKEY_DISPLAY:
	case PASSKEY_INPUT:
		err = sc_smp_check_confirm(smp);
		if (err) {
			return err;
		}

		atomic_set_bit(smp->allowed_cmds,
			       BT_SMP_CMD_PAIRING_CONFIRM);
		err = smp_send_pairing_random(smp);
		if (err) {
			return err;
		}

		smp->passkey_round++;
		if (smp->passkey_round == 20U) {
			atomic_set_bit(smp->allowed_cmds, BT_SMP_DHKEY_CHECK);
			atomic_set_bit(smp->flags, SMP_FLAG_DHCHECK_WAIT);
			return 0;
		}

		if (bt_rand(smp->prnd, 16)) {
			return BT_SMP_ERR_UNSPECIFIED;
		}

		return 0;
	case LE_SC_OOB:
		/* Step 6: Select random N */
		if (bt_rand(smp->prnd, 16)) {
			return BT_SMP_ERR_UNSPECIFIED;
		}

		if (smp_auth_cb && smp_auth_cb->oob_data_request) {
			struct bt_conn_oob_info info = {
				.type = BT_CONN_OOB_LE_SC,
				.lesc.oob_config = BT_CONN_OOB_NO_DATA,
			};

			le_sc_oob_config_set(smp, &info);

			smp->oobd_local = NULL;
			smp->oobd_remote = NULL;

			atomic_set_bit(smp->flags, SMP_FLAG_OOB_PENDING);
			smp_auth_cb->oob_data_request(smp->chan.chan.conn, &info);

			return 0;
		} else {
			return BT_SMP_ERR_OOB_NOT_AVAIL;
		}
	default:
		LOG_ERR("Unknown pairing method (%u)", smp->method);
		return BT_SMP_ERR_UNSPECIFIED;
	}

	atomic_set_bit(smp->allowed_cmds, BT_SMP_DHKEY_CHECK);
	atomic_set_bit(smp->flags, SMP_FLAG_DHCHECK_WAIT);
	return smp_send_pairing_random(smp);
#else
	return BT_SMP_ERR_PAIRING_NOTSUPP;
#endif /* CONFIG_BT_PERIPHERAL */
}

static uint8_t smp_pairing_failed(struct bt_smp *smp, struct net_buf *buf)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	const struct bt_conn_auth_cb *smp_auth_cb = latch_auth_cb(smp);
	struct bt_smp_pairing_fail *req = (void *)buf->data;

	LOG_ERR("pairing failed (peer reason 0x%x)", req->reason);

	if (atomic_test_and_clear_bit(smp->flags, SMP_FLAG_USER) ||
	    atomic_test_and_clear_bit(smp->flags, SMP_FLAG_DISPLAY)) {
		if (smp_auth_cb && smp_auth_cb->cancel) {
			smp_auth_cb->cancel(conn);
		}
	}

	smp_pairing_complete(smp, req->reason);

	/* return no error to avoid sending Pairing Failed in response */
	return 0;
}

static uint8_t smp_ident_info(struct bt_smp *smp, struct net_buf *buf)
{
	LOG_DBG("");

	if (atomic_test_bit(smp->flags, SMP_FLAG_BOND)) {
		struct bt_smp_ident_info *req = (void *)buf->data;
		struct bt_conn *conn = smp->chan.chan.conn;
		struct bt_keys *keys;

		keys = bt_keys_get_type(BT_KEYS_IRK, conn->id, &conn->le.dst);
		if (!keys) {
			LOG_ERR("Unable to get keys for %s", bt_addr_le_str(&conn->le.dst));
			return BT_SMP_ERR_UNSPECIFIED;
		}

		memcpy(keys->irk.val, req->irk, 16);
	}

	atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_IDENT_ADDR_INFO);

	return 0;
}

static uint8_t smp_id_add_replace(struct bt_smp *smp, struct bt_keys *new_bond)
{
	struct bt_keys *conflict;

	/* Sanity check: It does not make sense to finalize a bond before we
	 * have the remote identity.
	 */
	__ASSERT_NO_MSG(!(smp->remote_dist & BT_SMP_DIST_ID_KEY));

	conflict = bt_id_find_conflict(new_bond);
	if (conflict) {
		LOG_DBG("New bond conflicts with a bond on id %d.", conflict->id);
	}

	if (conflict && !IS_ENABLED(CONFIG_BT_ID_UNPAIR_MATCHING_BONDS)) {
		LOG_WRN("Refusing new pairing. The old bond must be unpaired first.");
		return BT_SMP_ERR_AUTH_REQUIREMENTS;
	}

	if (conflict && IS_ENABLED(CONFIG_BT_ID_UNPAIR_MATCHING_BONDS)) {
		bool trust_ok;
		int unpair_err;

		trust_ok = update_keys_check(smp, conflict);
		if (!trust_ok) {
			LOG_WRN("Refusing new pairing. The old bond has more trust.");
			return BT_SMP_ERR_AUTH_REQUIREMENTS;
		}

		LOG_DBG("Un-pairing old conflicting bond and finalizing new.");

		unpair_err = bt_unpair(conflict->id, &conflict->addr);
		__ASSERT_NO_MSG(!unpair_err);
	}

	__ASSERT_NO_MSG(!bt_id_find_conflict(new_bond));
	bt_id_add(new_bond);
	return 0;
}

struct addr_match {
	const bt_addr_le_t *rpa;
	const bt_addr_le_t *id_addr;
};

static void convert_to_id_on_match(struct bt_conn *conn, void *data)
{
	struct addr_match *addr_match = data;

	if (bt_addr_le_eq(&conn->le.dst, addr_match->rpa)) {
		bt_addr_le_copy(&conn->le.dst, addr_match->id_addr);
	}
}

static uint8_t smp_ident_addr_info(struct bt_smp *smp, struct net_buf *buf)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	struct bt_smp_ident_addr_info *req = (void *)buf->data;
	uint8_t err;

	LOG_DBG("identity %s", bt_addr_le_str(&req->addr));

	smp->remote_dist &= ~BT_SMP_DIST_ID_KEY;

	if (!bt_addr_le_is_identity(&req->addr)) {
		LOG_ERR("Invalid identity %s", bt_addr_le_str(&req->addr));
		LOG_ERR(" for %s", bt_addr_le_str(&conn->le.dst));
		return BT_SMP_ERR_INVALID_PARAMS;
	}

	if (!bt_addr_le_eq(&conn->le.dst, &req->addr)) {
		struct bt_keys *keys = bt_keys_find_addr(conn->id, &req->addr);

		if (keys) {
			if (!update_keys_check(smp, keys)) {
				return BT_SMP_ERR_UNSPECIFIED;
			}

			bt_keys_clear(keys);
		}
	}

	if (atomic_test_bit(smp->flags, SMP_FLAG_BOND)) {
		const bt_addr_le_t *dst;
		struct bt_keys *keys;

		keys = bt_keys_get_type(BT_KEYS_IRK, conn->id, &conn->le.dst);
		if (!keys) {
			LOG_ERR("Unable to get keys for %s", bt_addr_le_str(&conn->le.dst));
			return BT_SMP_ERR_UNSPECIFIED;
		}

		/*
		 * We can't use conn->dst here as this might already contain
		 * identity address known from previous pairing. Since all keys
		 * are cleared on re-pairing we wouldn't store IRK distributed
		 * in new pairing.
		 */
		if (conn->role == BT_HCI_ROLE_CENTRAL) {
			dst = &conn->le.resp_addr;
		} else {
			dst = &conn->le.init_addr;
		}

		if (bt_addr_le_is_rpa(dst)) {
			/* always update last use RPA */
			bt_addr_copy(&keys->irk.rpa, &dst->a);

			/*
			 * Update connection address and notify about identity
			 * resolved only if connection wasn't already reported
			 * with identity address. This may happen if IRK was
			 * present before ie. due to re-pairing.
			 */
			if (!bt_addr_le_is_identity(&conn->le.dst)) {
				struct addr_match addr_match = {
					.rpa = &conn->le.dst,
					.id_addr = &req->addr,
				};

				bt_conn_foreach(BT_CONN_TYPE_LE,
						convert_to_id_on_match,
						&addr_match);
				bt_addr_le_copy(&keys->addr, &req->addr);

				bt_conn_identity_resolved(conn);
			}
		}

		err = smp_id_add_replace(smp, keys);
		if (err) {
			return err;
		}
	}

	if (smp->remote_dist & BT_SMP_DIST_SIGN) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_SIGNING_INFO);
	}

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    conn->role == BT_HCI_ROLE_CENTRAL && !smp->remote_dist) {
		err = bt_smp_distribute_keys(smp);
		if (err) {
			return err;
		}
	}

	/* if all keys were distributed, pairing is done */
	if (!smp->local_dist && !smp->remote_dist) {
		smp_pairing_complete(smp, 0);
	}

	return 0;
}

#if defined(CONFIG_BT_SIGNING)
static uint8_t smp_signing_info(struct bt_smp *smp, struct net_buf *buf)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	uint8_t err;

	LOG_DBG("");

	if (atomic_test_bit(smp->flags, SMP_FLAG_BOND)) {
		struct bt_smp_signing_info *req = (void *)buf->data;
		struct bt_keys *keys;

		keys = bt_keys_get_type(BT_KEYS_REMOTE_CSRK, conn->id,
					&conn->le.dst);
		if (!keys) {
			LOG_ERR("Unable to get keys for %s", bt_addr_le_str(&conn->le.dst));
			return BT_SMP_ERR_UNSPECIFIED;
		}

		memcpy(keys->remote_csrk.val, req->csrk,
		       sizeof(keys->remote_csrk.val));
	}

	smp->remote_dist &= ~BT_SMP_DIST_SIGN;

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    conn->role == BT_HCI_ROLE_CENTRAL && !smp->remote_dist) {
		err = bt_smp_distribute_keys(smp);
		if (err) {
			return err;
		}
	}

	/* if all keys were distributed, pairing is done */
	if (!smp->local_dist && !smp->remote_dist) {
		smp_pairing_complete(smp, 0);
	}

	return 0;
}
#else
static uint8_t smp_signing_info(struct bt_smp *smp, struct net_buf *buf)
{
	return BT_SMP_ERR_CMD_NOTSUPP;
}
#endif /* CONFIG_BT_SIGNING */

#if defined(CONFIG_BT_CENTRAL)
static uint8_t smp_security_request(struct bt_smp *smp, struct net_buf *buf)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	struct bt_smp_security_request *req = (void *)buf->data;
	uint8_t auth;

	LOG_DBG("");

	/* A higher security level is requested during the key distribution
	 * phase, once pairing is complete a new pairing procedure will start.
	 */
	if (atomic_test_bit(smp->flags, SMP_FLAG_KEYS_DISTR)) {
		return 0;
	}

	if (atomic_test_bit(smp->flags, SMP_FLAG_PAIRING)) {
		/* We have already started pairing process */
		return 0;
	}

	if (atomic_test_bit(smp->flags, SMP_FLAG_ENC_PENDING)) {
		/* We have already started encryption procedure */
		return 0;
	}

	if (sc_supported) {
		auth = req->auth_req & BT_SMP_AUTH_MASK_SC;
	} else {
		auth = req->auth_req & BT_SMP_AUTH_MASK;
	}

	if (IS_ENABLED(CONFIG_BT_SMP_SC_PAIR_ONLY) &&
	    !(auth & BT_SMP_AUTH_SC)) {
		return BT_SMP_ERR_AUTH_REQUIREMENTS;
	}

	if (IS_ENABLED(CONFIG_BT_BONDING_REQUIRED) &&
	    !(latch_bondable(smp) && (auth & BT_SMP_AUTH_BONDING))) {
		/* Reject security req if not both intend to bond */
		LOG_DBG("Bonding required");
		return BT_SMP_ERR_UNSPECIFIED;
	}

	if (conn->le.keys) {
		/* Make sure we have an LTK to encrypt with */
		if (!(conn->le.keys->keys & (BT_KEYS_LTK_P256 | BT_KEYS_LTK))) {
			goto pair;
		}
	} else {
		conn->le.keys = bt_keys_find(BT_KEYS_LTK_P256, conn->id,
					     &conn->le.dst);
		if (!conn->le.keys) {
			conn->le.keys = bt_keys_find(BT_KEYS_LTK, conn->id,
						     &conn->le.dst);
		}
	}

	if (!conn->le.keys) {
		goto pair;
	}

	/* if MITM required key must be authenticated */
	if ((auth & BT_SMP_AUTH_MITM) &&
	    !(conn->le.keys->flags & BT_KEYS_AUTHENTICATED)) {
		if (get_io_capa(smp) != BT_SMP_IO_NO_INPUT_OUTPUT) {
			LOG_INF("New auth requirements: 0x%x, repairing", auth);
			goto pair;
		}

		LOG_WRN("Unsupported auth requirements: 0x%x, repairing", auth);
		goto pair;
	}

	/* if LE SC required and no p256 key present repair */
	if ((auth & BT_SMP_AUTH_SC) &&
	    !(conn->le.keys->keys & BT_KEYS_LTK_P256)) {
		LOG_INF("New auth requirements: 0x%x, repairing", auth);
		goto pair;
	}

	if (bt_conn_le_start_encryption(conn, conn->le.keys->ltk.rand,
					conn->le.keys->ltk.ediv,
					conn->le.keys->ltk.val,
					conn->le.keys->enc_size) < 0) {
		LOG_ERR("Failed to start encryption");
		return BT_SMP_ERR_UNSPECIFIED;
	}

	atomic_set_bit(smp->flags, SMP_FLAG_ENC_PENDING);

	return 0;
pair:
	if (smp_send_pairing_req(conn) < 0) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	atomic_set_bit(smp->flags, SMP_FLAG_SEC_REQ);

	return 0;
}
#else
static uint8_t smp_security_request(struct bt_smp *smp, struct net_buf *buf)
{
	return BT_SMP_ERR_CMD_NOTSUPP;
}
#endif /* CONFIG_BT_CENTRAL */

#ifndef CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY
static uint8_t generate_dhkey(struct bt_smp *smp)
{
	if (IS_ENABLED(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	atomic_set_bit(smp->flags, SMP_FLAG_DHKEY_PENDING);
	if (!smp_find(SMP_FLAG_DHKEY_GEN)) {
		return smp_dhkey_generate(smp);
	}

	return 0;
}

static uint8_t display_passkey(struct bt_smp *smp)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	const struct bt_conn_auth_cb *smp_auth_cb = latch_auth_cb(smp);

	if (IS_ENABLED(CONFIG_BT_FIXED_PASSKEY) &&
	    fixed_passkey != BT_PASSKEY_INVALID) {
		smp->passkey = fixed_passkey;
	} else {
		if (bt_rand(&smp->passkey, sizeof(smp->passkey))) {
			return BT_SMP_ERR_UNSPECIFIED;
		}

		smp->passkey %= 1000000;
	}

	smp->passkey_round = 0U;

	if (smp_auth_cb && smp_auth_cb->passkey_display) {
		atomic_set_bit(smp->flags, SMP_FLAG_DISPLAY);
		smp_auth_cb->passkey_display(conn, smp->passkey);
	}

	smp->passkey = sys_cpu_to_le32(smp->passkey);

	return 0;
}
#endif /* CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY */

#if defined(CONFIG_BT_PERIPHERAL)
static uint8_t smp_public_key_periph(struct bt_smp *smp)
{
	const struct bt_conn_auth_cb *smp_auth_cb = latch_auth_cb(smp);
	uint8_t err;

	if (!atomic_test_bit(smp->flags, SMP_FLAG_SC_DEBUG_KEY) &&
	    memcmp(smp->pkey, sc_public_key, BT_PUB_KEY_COORD_LEN) == 0) {
		/* Deny public key with identitcal X coordinate unless it is the
		 * debug public key.
		 */
		LOG_WRN("Remote public key rejected");
		return BT_SMP_ERR_UNSPECIFIED;
	}

	err = sc_send_public_key(smp);
	if (err) {
		return err;
	}

	switch (smp->method) {
	case PASSKEY_CONFIRM:
	case JUST_WORKS:
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_RANDOM);

		err = smp_send_pairing_confirm(smp);
		if (err) {
			return err;
		}
		break;
	case PASSKEY_DISPLAY:
		err = display_passkey(smp);
		if (err) {
			return err;
		}

		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_CONFIRM);
		atomic_set_bit(smp->allowed_cmds, BT_SMP_KEYPRESS_NOTIFICATION);
		break;
	case PASSKEY_INPUT:
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_CONFIRM);
		atomic_set_bit(smp->allowed_cmds, BT_SMP_KEYPRESS_NOTIFICATION);
		atomic_set_bit(smp->flags, SMP_FLAG_USER);
		smp_auth_cb->passkey_entry(smp->chan.chan.conn);
		break;
	case LE_SC_OOB:
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_RANDOM);
		break;
	default:
		LOG_ERR("Unknown pairing method (%u)", smp->method);
		return BT_SMP_ERR_UNSPECIFIED;
	}

	return generate_dhkey(smp);
}
#endif /* CONFIG_BT_PERIPHERAL */

#ifdef CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY
static uint8_t smp_public_key(struct bt_smp *smp, struct net_buf *buf)
{
	return BT_SMP_ERR_AUTH_REQUIREMENTS;
}
#else
static uint8_t smp_public_key(struct bt_smp *smp, struct net_buf *buf)
{
	const struct bt_conn_auth_cb *smp_auth_cb = latch_auth_cb(smp);
	struct bt_smp_public_key *req = (void *)buf->data;
	uint8_t err;

	LOG_DBG("");

	memcpy(smp->pkey, req->x, BT_PUB_KEY_COORD_LEN);
	memcpy(&smp->pkey[BT_PUB_KEY_COORD_LEN], req->y, BT_PUB_KEY_COORD_LEN);

	/* mark key as debug if remote is using it */
	if (bt_pub_key_is_debug(smp->pkey)) {
		LOG_INF("Remote is using Debug Public key");
		atomic_set_bit(smp->flags, SMP_FLAG_SC_DEBUG_KEY);

		/* Don't allow a bond established without debug key to be
		 * updated using LTK generated from debug key.
		 */
		if (!update_debug_keys_check(smp)) {
			return BT_SMP_ERR_AUTH_REQUIREMENTS;
		}
	}

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    smp->chan.chan.conn->role == BT_HCI_ROLE_CENTRAL) {
		if (!atomic_test_bit(smp->flags, SMP_FLAG_SC_DEBUG_KEY) &&
		    memcmp(smp->pkey, sc_public_key, BT_PUB_KEY_COORD_LEN) == 0) {
			/* Deny public key with identitcal X coordinate unless
			 * it is the debug public key.
			 */
			LOG_WRN("Remote public key rejected");
			return BT_SMP_ERR_UNSPECIFIED;
		}

		switch (smp->method) {
		case PASSKEY_CONFIRM:
		case JUST_WORKS:
			atomic_set_bit(smp->allowed_cmds,
				       BT_SMP_CMD_PAIRING_CONFIRM);
			break;
		case PASSKEY_DISPLAY:
			err = display_passkey(smp);
			if (err) {
				return err;
			}

			atomic_set_bit(smp->allowed_cmds,
				       BT_SMP_CMD_PAIRING_CONFIRM);

			atomic_set_bit(smp->allowed_cmds,
				       BT_SMP_KEYPRESS_NOTIFICATION);

			err = smp_send_pairing_confirm(smp);
			if (err) {
				return err;
			}
			break;
		case PASSKEY_INPUT:
			atomic_set_bit(smp->flags, SMP_FLAG_USER);
			smp_auth_cb->passkey_entry(smp->chan.chan.conn);

			atomic_set_bit(smp->allowed_cmds,
				       BT_SMP_KEYPRESS_NOTIFICATION);

			break;
		case LE_SC_OOB:
			/* Step 6: Select random N */
			if (bt_rand(smp->prnd, 16)) {
				return BT_SMP_ERR_UNSPECIFIED;
			}

			if (smp_auth_cb && smp_auth_cb->oob_data_request) {
				struct bt_conn_oob_info info = {
					.type = BT_CONN_OOB_LE_SC,
					.lesc.oob_config = BT_CONN_OOB_NO_DATA,
				};

				le_sc_oob_config_set(smp, &info);

				smp->oobd_local = NULL;
				smp->oobd_remote = NULL;

				atomic_set_bit(smp->flags,
					       SMP_FLAG_OOB_PENDING);
				smp_auth_cb->oob_data_request(smp->chan.chan.conn, &info);
			} else {
				return BT_SMP_ERR_OOB_NOT_AVAIL;
			}
			break;
		default:
			LOG_ERR("Unknown pairing method (%u)", smp->method);
			return BT_SMP_ERR_UNSPECIFIED;
		}

		return generate_dhkey(smp);
	}

#if defined(CONFIG_BT_PERIPHERAL)
	if (!sc_public_key) {
		atomic_set_bit(smp->flags, SMP_FLAG_PKEY_SEND);
		return 0;
	}

	err = smp_public_key_periph(smp);
	if (err) {
		return err;
	}
#endif /* CONFIG_BT_PERIPHERAL */

	return 0;
}
#endif /* CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY */

static uint8_t smp_dhkey_check(struct bt_smp *smp, struct net_buf *buf)
{
	struct bt_smp_dhkey_check *req = (void *)buf->data;

	LOG_DBG("");

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    smp->chan.chan.conn->role == BT_HCI_ROLE_CENTRAL) {
		uint8_t e[16], r[16], enc_size;
		uint8_t ediv[2], rand[8];

		(void)memset(r, 0, sizeof(r));

		switch (smp->method) {
		case JUST_WORKS:
		case PASSKEY_CONFIRM:
			break;
		case PASSKEY_DISPLAY:
		case PASSKEY_INPUT:
			memcpy(r, &smp->passkey, sizeof(smp->passkey));
			break;
		case LE_SC_OOB:
			if (smp->oobd_local) {
				memcpy(r, smp->oobd_local->r, sizeof(r));
			}
			break;
		default:
			LOG_ERR("Unknown pairing method (%u)", smp->method);
			return BT_SMP_ERR_UNSPECIFIED;
		}

		/* calculate remote DHKey check for comparison */
		if (bt_crypto_f6(smp->mackey, smp->rrnd, smp->prnd, r, &smp->prsp[1],
				 &smp->chan.chan.conn->le.resp_addr,
				 &smp->chan.chan.conn->le.init_addr, e)) {
			return BT_SMP_ERR_UNSPECIFIED;
		}

		if (memcmp(e, req->e, 16)) {
			return BT_SMP_ERR_DHKEY_CHECK_FAILED;
		}

		enc_size = get_encryption_key_size(smp);

		/* Rand and EDiv are 0 */
		(void)memset(ediv, 0, sizeof(ediv));
		(void)memset(rand, 0, sizeof(rand));
		if (bt_conn_le_start_encryption(smp->chan.chan.conn, rand, ediv,
						smp->tk, enc_size) < 0) {
			LOG_ERR("Failed to start encryption");
			return BT_SMP_ERR_UNSPECIFIED;
		}

		atomic_set_bit(smp->flags, SMP_FLAG_ENC_PENDING);

		if (IS_ENABLED(CONFIG_BT_SMP_USB_HCI_CTLR_WORKAROUND)) {
			if (smp->remote_dist & BT_SMP_DIST_ID_KEY) {
				atomic_set_bit(smp->allowed_cmds,
					       BT_SMP_CMD_IDENT_INFO);
			} else if (smp->remote_dist & BT_SMP_DIST_SIGN) {
				atomic_set_bit(smp->allowed_cmds,
					       BT_SMP_CMD_SIGNING_INFO);
			}
		}

		return 0;
	}

#if defined(CONFIG_BT_PERIPHERAL)
	if (smp->chan.chan.conn->role == BT_HCI_ROLE_PERIPHERAL) {
		atomic_clear_bit(smp->flags, SMP_FLAG_DHCHECK_WAIT);
		memcpy(smp->e, req->e, sizeof(smp->e));

		/* wait for DHKey being generated */
		if (atomic_test_bit(smp->flags, SMP_FLAG_DHKEY_PENDING)) {
			atomic_set_bit(smp->flags, SMP_FLAG_DHKEY_SEND);
			return 0;
		}

		/* waiting for user to confirm passkey */
		if (atomic_test_bit(smp->flags, SMP_FLAG_USER)) {
			atomic_set_bit(smp->flags, SMP_FLAG_DHKEY_SEND);
			return 0;
		}

		return compute_and_check_and_send_periph_dhcheck(smp);
	}
#endif /* CONFIG_BT_PERIPHERAL */

	return 0;
}

#if defined(CONFIG_BT_PASSKEY_KEYPRESS)
static uint8_t smp_keypress_notif(struct bt_smp *smp, struct net_buf *buf)
{
	const struct bt_conn_auth_cb *smp_auth_cb = latch_auth_cb(smp);
	struct bt_conn *conn = smp->chan.chan.conn;
	struct bt_smp_keypress_notif *notif = (void *)buf->data;
	enum bt_conn_auth_keypress type = notif->type;

	LOG_DBG("Keypress from conn %u, type %u", bt_conn_index(conn), type);

	/* For now, keypress notifications are always accepted. In the future we
	 * should be smarter about this. We might also want to enforce something
	 * about the 'start' and 'end' messages.
	 */
	atomic_set_bit(smp->allowed_cmds, BT_SMP_KEYPRESS_NOTIFICATION);

	if (!IN_RANGE(type,
		      BT_CONN_AUTH_KEYPRESS_ENTRY_STARTED,
		      BT_CONN_AUTH_KEYPRESS_ENTRY_COMPLETED)) {
		LOG_WRN("Received unknown keypress event type %u. Discarding.", type);
		return BT_SMP_ERR_INVALID_PARAMS;
	}

	/* Reset SMP timeout, like the spec says. */
	k_work_reschedule(&smp->work, SMP_TIMEOUT);

	if (smp_auth_cb->passkey_display_keypress) {
		smp_auth_cb->passkey_display_keypress(conn, type);
	}

	return 0;
}
#else
static uint8_t smp_keypress_notif(struct bt_smp *smp, struct net_buf *buf)
{
	ARG_UNUSED(smp);
	ARG_UNUSED(buf);

	LOG_DBG("");

	/* Ignore packets until keypress notifications are fully supported. */
	atomic_set_bit(smp->allowed_cmds, BT_SMP_KEYPRESS_NOTIFICATION);
	return 0;
}
#endif

static const struct {
	uint8_t  (*func)(struct bt_smp *smp, struct net_buf *buf);
	uint8_t  expect_len;
} handlers[] = {
	{ }, /* No op-code defined for 0x00 */
	{ smp_pairing_req,         sizeof(struct bt_smp_pairing) },
	{ smp_pairing_rsp,         sizeof(struct bt_smp_pairing) },
	{ smp_pairing_confirm,     sizeof(struct bt_smp_pairing_confirm) },
	{ smp_pairing_random,      sizeof(struct bt_smp_pairing_random) },
	{ smp_pairing_failed,      sizeof(struct bt_smp_pairing_fail) },
	{ smp_encrypt_info,        sizeof(struct bt_smp_encrypt_info) },
	{ smp_central_ident,       sizeof(struct bt_smp_central_ident) },
	{ smp_ident_info,          sizeof(struct bt_smp_ident_info) },
	{ smp_ident_addr_info,     sizeof(struct bt_smp_ident_addr_info) },
	{ smp_signing_info,        sizeof(struct bt_smp_signing_info) },
	{ smp_security_request,    sizeof(struct bt_smp_security_request) },
	{ smp_public_key,          sizeof(struct bt_smp_public_key) },
	{ smp_dhkey_check,         sizeof(struct bt_smp_dhkey_check) },
	{ smp_keypress_notif,      sizeof(struct bt_smp_keypress_notif) },
};

static bool is_in_pairing_procedure(struct bt_smp *smp)
{
	return atomic_test_bit(smp->flags, SMP_FLAG_PAIRING);
}

static int bt_smp_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_smp *smp = CONTAINER_OF(chan, struct bt_smp, chan.chan);
	struct bt_smp_hdr *hdr;
	uint8_t err;

	if (buf->len < sizeof(*hdr)) {
		LOG_ERR("Too small SMP PDU received");
		return 0;
	}

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	LOG_DBG("Received SMP code 0x%02x len %u", hdr->code, buf->len);

	/*
	 * If SMP timeout occurred "no further SMP commands shall be sent over
	 * the L2CAP Security Manager Channel. A new SM procedure shall only be
	 * performed when a new physical link has been established."
	 */
	if (atomic_test_bit(smp->flags, SMP_FLAG_TIMEOUT)) {
		LOG_WRN("SMP command (code 0x%02x) received after timeout", hdr->code);
		return 0;
	}

	/*
	 * Bluetooth Core Specification Version 5.2, Vol 3, Part H, page 1667:
	 * If a packet is received with a Code that is reserved for future use
	 * it shall be ignored.
	 */
	if (hdr->code >= ARRAY_SIZE(handlers)) {
		LOG_WRN("Received reserved SMP code 0x%02x", hdr->code);
		return 0;
	}

	if (!handlers[hdr->code].func) {
		LOG_WRN("Unhandled SMP code 0x%02x", hdr->code);
		smp_error(smp, BT_SMP_ERR_CMD_NOTSUPP);
		return 0;
	}

	if (!atomic_test_and_clear_bit(smp->allowed_cmds, hdr->code)) {
		LOG_WRN("Unexpected SMP code 0x%02x", hdr->code);
		/* Do not send errors outside of pairing procedure. */
		if (is_in_pairing_procedure(smp)) {
			smp_error(smp, BT_SMP_ERR_UNSPECIFIED);
		}
		return 0;
	}

	if (buf->len != handlers[hdr->code].expect_len) {
		LOG_ERR("Invalid len %u for code 0x%02x", buf->len, hdr->code);
		smp_error(smp, BT_SMP_ERR_INVALID_PARAMS);
		return 0;
	}

	err = handlers[hdr->code].func(smp, buf);
	if (err) {
		smp_error(smp, err);
	}

	return 0;
}

static void bt_smp_pkey_ready(const uint8_t *pkey)
{
	int i;

	LOG_DBG("");

	sc_public_key = pkey;

	if (!pkey) {
		LOG_WRN("Public key not available");
		return;
	}

	k_sem_give(&sc_local_pkey_ready);

	for (i = 0; i < ARRAY_SIZE(bt_smp_pool); i++) {
		struct bt_smp *smp = &bt_smp_pool[i];
		uint8_t err;

		if (!atomic_test_bit(smp->flags, SMP_FLAG_PKEY_SEND)) {
			continue;
		}

		if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
		    smp->chan.chan.conn->role == BT_HCI_ROLE_CENTRAL) {
			err = sc_send_public_key(smp);
			if (err) {
				smp_error(smp, err);
			}

			atomic_set_bit(smp->allowed_cmds,
				       BT_SMP_CMD_PUBLIC_KEY);
			continue;
		}

#if defined(CONFIG_BT_PERIPHERAL)
		err = smp_public_key_periph(smp);
		if (err) {
			smp_error(smp, err);
		}
#endif /* CONFIG_BT_PERIPHERAL */
	}
}

static void bt_smp_connected(struct bt_l2cap_chan *chan)
{
	struct bt_smp *smp = CONTAINER_OF(chan, struct bt_smp, chan.chan);

	LOG_DBG("chan %p cid 0x%04x", chan,
		CONTAINER_OF(chan, struct bt_l2cap_le_chan, chan)->tx.cid);

	k_work_init_delayable(&smp->work, smp_timeout);
	smp_reset(smp);

	atomic_ptr_set(&smp->auth_cb, BT_SMP_AUTH_CB_UNINITIALIZED);
	atomic_set(&smp->bondable, BT_SMP_BONDABLE_UNINITIALIZED);
}

static void bt_smp_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_smp *smp = CONTAINER_OF(chan, struct bt_smp, chan.chan);
	struct bt_keys *keys = chan->conn->le.keys;

	LOG_DBG("chan %p cid 0x%04x", chan,
		CONTAINER_OF(chan, struct bt_l2cap_le_chan, chan)->tx.cid);

	/* Channel disconnected callback is always called from a work handler
	 * so canceling of the timeout work should always succeed.
	 */
	(void)k_work_cancel_delayable(&smp->work);

	if (atomic_test_bit(smp->flags, SMP_FLAG_PAIRING) ||
	    atomic_test_bit(smp->flags, SMP_FLAG_ENC_PENDING) ||
	    atomic_test_bit(smp->flags, SMP_FLAG_SEC_REQ)) {
		/* reset context and report */
		smp_pairing_complete(smp, BT_SMP_ERR_UNSPECIFIED);
	}

	if (keys) {
		/*
		 * If debug keys were used for pairing remove them.
		 * No keys indicate no bonding so free keys storage.
		 */
		if (!keys->keys || (!IS_ENABLED(CONFIG_BT_STORE_DEBUG_KEYS) &&
		    (keys->flags & BT_KEYS_DEBUG))) {
			bt_keys_clear(keys);
		}
	}

	(void)memset(smp, 0, sizeof(*smp));
}

static void bt_smp_encrypt_change(struct bt_l2cap_chan *chan,
				  uint8_t hci_status)
{
	struct bt_smp *smp = CONTAINER_OF(chan, struct bt_smp, chan.chan);
	struct bt_conn *conn = chan->conn;

	LOG_DBG("chan %p conn %p handle %u encrypt 0x%02x hci status 0x%02x", chan, conn,
		conn->handle, conn->encrypt, hci_status);

	if (!atomic_test_and_clear_bit(smp->flags, SMP_FLAG_ENC_PENDING)) {
		/* We where not waiting for encryption procedure.
		 * This happens when encrypt change is called to notify that
		 * security has failed before starting encryption.
		 */
		return;
	}

	if (hci_status) {
		if (atomic_test_bit(smp->flags, SMP_FLAG_PAIRING)) {
			uint8_t smp_err = smp_err_get(
				bt_security_err_get(hci_status));

			/* Fail as if it happened during key distribution */
			atomic_set_bit(smp->flags, SMP_FLAG_KEYS_DISTR);
			smp_pairing_complete(smp, smp_err);
		}

		return;
	}

	if (!conn->encrypt) {
		return;
	}

	/* We were waiting for encryption but with no pairing in progress.
	 * This can happen if paired peripheral sent Security Request and we
	 * enabled encryption.
	 */
	if (!atomic_test_bit(smp->flags, SMP_FLAG_PAIRING)) {
		smp_reset(smp);
		return;
	}

	/* derive BR/EDR LinkKey if supported by both sides */
	if (atomic_test_bit(smp->flags, SMP_FLAG_SC)) {
		if ((smp->local_dist & BT_SMP_DIST_LINK_KEY) &&
		    (smp->remote_dist & BT_SMP_DIST_LINK_KEY)) {
			/*
			 * Link Key will be derived after key distribution to
			 * make sure remote device identity is known
			 */
			atomic_set_bit(smp->flags, SMP_FLAG_DERIVE_LK);
		}
		/*
		 * Those are used as pairing finished indicator so generated
		 * but not distributed keys must be cleared here.
		 */
		smp->local_dist &= ~BT_SMP_DIST_LINK_KEY;
		smp->remote_dist &= ~BT_SMP_DIST_LINK_KEY;
	}

	if (smp->remote_dist & BT_SMP_DIST_ENC_KEY) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_ENCRYPT_INFO);
	} else if (smp->remote_dist & BT_SMP_DIST_ID_KEY) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_IDENT_INFO);
	} else if (smp->remote_dist & BT_SMP_DIST_SIGN) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_SIGNING_INFO);
	}

	/* This is the last point that is common for all code paths in the
	 * pairing process (during which we still have the option to send
	 * Pairing Failed). That makes it convenient to update the RL here. We
	 * want to update the RL during the pairing process so that we can fail
	 * it in case there is a conflict with an existing bond.
	 *
	 * We can do the update here only if the peer does not intend to send us
	 * any identity information. In this case we already have everything
	 * that goes into the RL.
	 *
	 * We need an entry in the RL despite the remote not using privacy. This
	 * is because we are using privacy locally and need to associate correct
	 * local IRK with the peer.
	 *
	 * If the peer does intend to send us identity information, we must wait
	 * for that information to enter it in the RL. In that case, we call
	 * `smp_id_add_replace` not here, but later. If neither we nor the peer
	 * are using privacy, there is no need for an entry in the RL.
	 */
	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    IS_ENABLED(CONFIG_BT_PRIVACY) &&
	    conn->role == BT_HCI_ROLE_CENTRAL &&
	    !(smp->remote_dist & BT_SMP_DIST_ID_KEY)) {
		uint8_t smp_err;

		smp_err = smp_id_add_replace(smp, conn->le.keys);
		if (smp_err) {
			smp_pairing_complete(smp, smp_err);
		}
	}

	atomic_set_bit(smp->flags, SMP_FLAG_KEYS_DISTR);

	/* Peripheral distributes it's keys first */
	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    conn->role == BT_HCI_ROLE_CENTRAL && smp->remote_dist) {
		return;
	}

	if (IS_ENABLED(CONFIG_BT_TESTING)) {
		/* Avoid the HCI-USB race condition where HCI data and
		 * HCI events can be re-ordered, and pairing information appears
		 * to be sent unencrypted.
		 */
		k_sleep(K_MSEC(100));
	}

	if (bt_smp_distribute_keys(smp)) {
		return;
	}

	/* if all keys were distributed, pairing is done */
	if (!smp->local_dist && !smp->remote_dist) {
		smp_pairing_complete(smp, 0);
	}
}

#if defined(CONFIG_BT_SIGNING) || defined(CONFIG_BT_SMP_SELFTEST)
/* Sign message using msg as a buffer, len is a size of the message,
 * msg buffer contains message itself, 32 bit count and signature,
 * so total buffer size is len + 4 + 8 octets.
 * API is Little Endian to make it suitable for Bluetooth.
 */
static int smp_sign_buf(const uint8_t *key, uint8_t *msg, uint16_t len)
{
	uint8_t *m = msg;
	uint32_t cnt = UNALIGNED_GET((uint32_t *)&msg[len]);
	uint8_t *sig = msg + len;
	uint8_t key_s[16], tmp[16];
	int err;

	LOG_DBG("Signing msg %s len %u key %s", bt_hex(msg, len), len, bt_hex(key, 16));

	sys_mem_swap(m, len + sizeof(cnt));
	sys_memcpy_swap(key_s, key, 16);

	err = bt_crypto_aes_cmac(key_s, m, len + sizeof(cnt), tmp);
	if (err) {
		LOG_ERR("Data signing failed");
		return err;
	}

	sys_mem_swap(tmp, sizeof(tmp));
	memcpy(tmp + 4, &cnt, sizeof(cnt));

	/* Swap original message back */
	sys_mem_swap(m, len + sizeof(cnt));

	memcpy(sig, tmp + 4, 12);

	LOG_DBG("sig %s", bt_hex(sig, 12));

	return 0;
}
#endif

#if defined(CONFIG_BT_SIGNING)
int bt_smp_sign_verify(struct bt_conn *conn, struct net_buf *buf)
{
	struct bt_keys *keys;
	uint8_t sig[12];
	uint32_t cnt;
	int err;

	/* Store signature incl. count */
	memcpy(sig, net_buf_tail(buf) - sizeof(sig), sizeof(sig));

	keys = bt_keys_find(BT_KEYS_REMOTE_CSRK, conn->id, &conn->le.dst);
	if (!keys) {
		LOG_ERR("Unable to find Remote CSRK for %s", bt_addr_le_str(&conn->le.dst));
		return -ENOENT;
	}

	/* Copy signing count */
	cnt = sys_cpu_to_le32(keys->remote_csrk.cnt);
	memcpy(net_buf_tail(buf) - sizeof(sig), &cnt, sizeof(cnt));

	LOG_DBG("Sign data len %zu key %s count %u", buf->len - sizeof(sig),
		bt_hex(keys->remote_csrk.val, 16), keys->remote_csrk.cnt);

	err = smp_sign_buf(keys->remote_csrk.val, buf->data,
			   buf->len - sizeof(sig));
	if (err) {
		LOG_ERR("Unable to create signature for %s", bt_addr_le_str(&conn->le.dst));
		return -EIO;
	}

	if (memcmp(sig, net_buf_tail(buf) - sizeof(sig), sizeof(sig))) {
		LOG_ERR("Unable to verify signature for %s", bt_addr_le_str(&conn->le.dst));
		return -EBADMSG;
	}

	keys->remote_csrk.cnt++;

	return 0;
}

int bt_smp_sign(struct bt_conn *conn, struct net_buf *buf)
{
	struct bt_keys *keys;
	uint32_t cnt;
	int err;

	keys = bt_keys_find(BT_KEYS_LOCAL_CSRK, conn->id, &conn->le.dst);
	if (!keys) {
		LOG_ERR("Unable to find local CSRK for %s", bt_addr_le_str(&conn->le.dst));
		return -ENOENT;
	}

	/* Reserve space for data signature */
	net_buf_add(buf, 12);

	/* Copy signing count */
	cnt = sys_cpu_to_le32(keys->local_csrk.cnt);
	memcpy(net_buf_tail(buf) - 12, &cnt, sizeof(cnt));

	LOG_DBG("Sign data len %u key %s count %u", buf->len, bt_hex(keys->local_csrk.val, 16),
		keys->local_csrk.cnt);

	err = smp_sign_buf(keys->local_csrk.val, buf->data, buf->len - 12);
	if (err) {
		LOG_ERR("Unable to create signature for %s", bt_addr_le_str(&conn->le.dst));
		return -EIO;
	}

	keys->local_csrk.cnt++;

	return 0;
}
#else
int bt_smp_sign_verify(struct bt_conn *conn, struct net_buf *buf)
{
	return -ENOTSUP;
}

int bt_smp_sign(struct bt_conn *conn, struct net_buf *buf)
{
	return -ENOTSUP;
}
#endif /* CONFIG_BT_SIGNING */

static int smp_d1(const uint8_t *key, uint16_t d, uint16_t r, uint8_t res[16])
{
	int err;

	LOG_DBG("key %s d %u r %u", bt_hex(key, 16), d, r);

	sys_put_le16(d, &res[0]);
	sys_put_le16(r, &res[2]);
	memset(&res[4], 0, 16 - 4);

	err = bt_encrypt_le(key, res, res);
	if (err) {
		return err;
	}

	LOG_DBG("res %s", bt_hex(res, 16));
	return 0;
}

int bt_smp_irk_get(uint8_t *ir, uint8_t *irk)
{
	uint8_t invalid_ir[16] = { 0 };

	if (!memcmp(ir, invalid_ir, 16)) {
		return -EINVAL;
	}

	return smp_d1(ir, 1, 0, irk);
}

#if defined(CONFIG_BT_SMP_SELFTEST)
/* Test vectors are taken from RFC 4493
 * https://tools.ietf.org/html/rfc4493
 * Same mentioned in the Bluetooth Spec.
 */
static const uint8_t key[] = {
	0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
	0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
};

static const uint8_t M[] = {
	0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
	0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
	0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
	0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51,
	0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
	0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef,
	0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
	0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10
};

static int aes_test(const char *prefix, const uint8_t *in_key, const uint8_t *m,
		    uint16_t len, const uint8_t *mac)
{
	uint8_t out[16];

	LOG_DBG("%s: AES CMAC of message with len %u", prefix, len);

	bt_crypto_aes_cmac(in_key, m, len, out);
	if (!memcmp(out, mac, 16)) {
		LOG_DBG("%s: Success", prefix);
	} else {
		LOG_ERR("%s: Failed", prefix);
		return -1;
	}

	return 0;
}

static int smp_aes_cmac_test(void)
{
	uint8_t mac1[] = {
		0xbb, 0x1d, 0x69, 0x29, 0xe9, 0x59, 0x37, 0x28,
		0x7f, 0xa3, 0x7d, 0x12, 0x9b, 0x75, 0x67, 0x46
	};
	uint8_t mac2[] = {
		0x07, 0x0a, 0x16, 0xb4, 0x6b, 0x4d, 0x41, 0x44,
		0xf7, 0x9b, 0xdd, 0x9d, 0xd0, 0x4a, 0x28, 0x7c
	};
	uint8_t mac3[] = {
		0xdf, 0xa6, 0x67, 0x47, 0xde, 0x9a, 0xe6, 0x30,
		0x30, 0xca, 0x32, 0x61, 0x14, 0x97, 0xc8, 0x27
	};
	uint8_t mac4[] = {
		0x51, 0xf0, 0xbe, 0xbf, 0x7e, 0x3b, 0x9d, 0x92,
		0xfc, 0x49, 0x74, 0x17, 0x79, 0x36, 0x3c, 0xfe
	};
	int err;

	err = aes_test("Test aes-cmac0", key, M, 0, mac1);
	if (err) {
		return err;
	}

	err = aes_test("Test aes-cmac16", key, M, 16, mac2);
	if (err) {
		return err;
	}

	err = aes_test("Test aes-cmac40", key, M, 40, mac3);
	if (err) {
		return err;
	}

	err = aes_test("Test aes-cmac64", key, M, 64, mac4);
	if (err) {
		return err;
	}

	return 0;
}

static int sign_test(const char *prefix, const uint8_t *sign_key, const uint8_t *m,
		     uint16_t len, const uint8_t *sig)
{
	uint8_t msg[len + sizeof(uint32_t) + 8];
	uint8_t orig[len + sizeof(uint32_t) + 8];
	uint8_t *out = msg + len;
	int err;

	LOG_DBG("%s: Sign message with len %u", prefix, len);

	(void)memset(msg, 0, sizeof(msg));
	memcpy(msg, m, len);
	(void)memset(msg + len, 0, sizeof(uint32_t));

	memcpy(orig, msg, sizeof(msg));

	err = smp_sign_buf(sign_key, msg, len);
	if (err) {
		return err;
	}

	/* Check original message */
	if (!memcmp(msg, orig, len + sizeof(uint32_t))) {
		LOG_DBG("%s: Original message intact", prefix);
	} else {
		LOG_ERR("%s: Original message modified", prefix);
		LOG_DBG("%s: orig %s", prefix, bt_hex(orig, sizeof(orig)));
		LOG_DBG("%s: msg %s", prefix, bt_hex(msg, sizeof(msg)));
		return -1;
	}

	if (!memcmp(out, sig, 12)) {
		LOG_DBG("%s: Success", prefix);
	} else {
		LOG_ERR("%s: Failed", prefix);
		return -1;
	}

	return 0;
}

static int smp_sign_test(void)
{
	const uint8_t sig1[] = {
		0x00, 0x00, 0x00, 0x00, 0xb3, 0xa8, 0x59, 0x41,
		0x27, 0xeb, 0xc2, 0xc0
	};
	const uint8_t sig2[] = {
		0x00, 0x00, 0x00, 0x00, 0x27, 0x39, 0x74, 0xf4,
		0x39, 0x2a, 0x23, 0x2a
	};
	const uint8_t sig3[] = {
		0x00, 0x00, 0x00, 0x00, 0xb7, 0xca, 0x94, 0xab,
		0x87, 0xc7, 0x82, 0x18
	};
	const uint8_t sig4[] = {
		0x00, 0x00, 0x00, 0x00, 0x44, 0xe1, 0xe6, 0xce,
		0x1d, 0xf5, 0x13, 0x68
	};
	uint8_t key_s[16];
	int err;

	/* Use the same key as aes-cmac but swap bytes */
	sys_memcpy_swap(key_s, key, 16);

	err = sign_test("Test sign0", key_s, M, 0, sig1);
	if (err) {
		return err;
	}

	err = sign_test("Test sign16", key_s, M, 16, sig2);
	if (err) {
		return err;
	}

	err = sign_test("Test sign40", key_s, M, 40, sig3);
	if (err) {
		return err;
	}

	err = sign_test("Test sign64", key_s, M, 64, sig4);
	if (err) {
		return err;
	}

	return 0;
}

static int smp_f4_test(void)
{
	uint8_t u[32] = { 0xe6, 0x9d, 0x35, 0x0e, 0x48, 0x01, 0x03, 0xcc,
			  0xdb, 0xfd, 0xf4, 0xac, 0x11, 0x91, 0xf4, 0xef,
			  0xb9, 0xa5, 0xf9, 0xe9, 0xa7, 0x83, 0x2c, 0x5e,
			  0x2c, 0xbe, 0x97, 0xf2, 0xd2, 0x03, 0xb0, 0x20 };
	uint8_t v[32] = { 0xfd, 0xc5, 0x7f, 0xf4, 0x49, 0xdd, 0x4f, 0x6b,
			  0xfb, 0x7c, 0x9d, 0xf1, 0xc2, 0x9a, 0xcb, 0x59,
			  0x2a, 0xe7, 0xd4, 0xee, 0xfb, 0xfc, 0x0a, 0x90,
			  0x9a, 0xbb, 0xf6, 0x32, 0x3d, 0x8b, 0x18, 0x55 };
	uint8_t x[16] = { 0xab, 0xae, 0x2b, 0x71, 0xec, 0xb2, 0xff, 0xff,
			  0x3e, 0x73, 0x77, 0xd1, 0x54, 0x84, 0xcb, 0xd5 };
	uint8_t z = 0x00;
	uint8_t exp[16] = { 0x2d, 0x87, 0x74, 0xa9, 0xbe, 0xa1, 0xed, 0xf1,
			    0x1c, 0xbd, 0xa9, 0x07, 0xf1, 0x16, 0xc9, 0xf2 };
	uint8_t res[16];
	int err;

	err = bt_crypto_f4(u, v, x, z, res);
	if (err) {
		return err;
	}

	if (memcmp(res, exp, 16)) {
		return -EINVAL;
	}

	return 0;
}

static int smp_f5_test(void)
{
	uint8_t w[32] = { 0x98, 0xa6, 0xbf, 0x73, 0xf3, 0x34, 0x8d, 0x86,
			  0xf1, 0x66, 0xf8, 0xb4, 0x13, 0x6b, 0x79, 0x99,
			  0x9b, 0x7d, 0x39, 0x0a, 0xa6, 0x10, 0x10, 0x34,
			  0x05, 0xad, 0xc8, 0x57, 0xa3, 0x34, 0x02, 0xec };
	uint8_t n1[16] = { 0xab, 0xae, 0x2b, 0x71, 0xec, 0xb2, 0xff, 0xff,
			   0x3e, 0x73, 0x77, 0xd1, 0x54, 0x84, 0xcb, 0xd5 };
	uint8_t n2[16] = { 0xcf, 0xc4, 0x3d, 0xff, 0xf7, 0x83, 0x65, 0x21,
			   0x6e, 0x5f, 0xa7, 0x25, 0xcc, 0xe7, 0xe8, 0xa6 };
	bt_addr_le_t a1 = { .type = 0x00,
			    .a.val = { 0xce, 0xbf, 0x37, 0x37, 0x12, 0x56 } };
	bt_addr_le_t a2 = { .type = 0x00,
			    .a.val = {0xc1, 0xcf, 0x2d, 0x70, 0x13, 0xa7 } };
	uint8_t exp_ltk[16] = { 0x38, 0x0a, 0x75, 0x94, 0xb5, 0x22, 0x05,
				0x98, 0x23, 0xcd, 0xd7, 0x69, 0x11, 0x79,
				0x86, 0x69 };
	uint8_t exp_mackey[16] = { 0x20, 0x6e, 0x63, 0xce, 0x20, 0x6a, 0x3f,
				   0xfd, 0x02, 0x4a, 0x08, 0xa1, 0x76, 0xf1,
				   0x65, 0x29 };
	uint8_t mackey[16], ltk[16];
	int err;

	err = bt_crypto_f5(w, n1, n2, &a1, &a2, mackey, ltk);
	if (err) {
		return err;
	}

	if (memcmp(mackey, exp_mackey, 16) || memcmp(ltk, exp_ltk, 16)) {
		return -EINVAL;
	}

	return 0;
}

static int smp_f6_test(void)
{
	uint8_t w[16] = { 0x20, 0x6e, 0x63, 0xce, 0x20, 0x6a, 0x3f, 0xfd,
			  0x02, 0x4a, 0x08, 0xa1, 0x76, 0xf1, 0x65, 0x29 };
	uint8_t n1[16] = { 0xab, 0xae, 0x2b, 0x71, 0xec, 0xb2, 0xff, 0xff,
			   0x3e, 0x73, 0x77, 0xd1, 0x54, 0x84, 0xcb, 0xd5 };
	uint8_t n2[16] = { 0xcf, 0xc4, 0x3d, 0xff, 0xf7, 0x83, 0x65, 0x21,
			   0x6e, 0x5f, 0xa7, 0x25, 0xcc, 0xe7, 0xe8, 0xa6 };
	uint8_t r[16] = { 0xc8, 0x0f, 0x2d, 0x0c, 0xd2, 0x42, 0xda, 0x08,
			  0x54, 0xbb, 0x53, 0xb4, 0x3b, 0x34, 0xa3, 0x12 };
	uint8_t io_cap[3] = { 0x02, 0x01, 0x01 };
	bt_addr_le_t a1 = { .type = 0x00,
			    .a.val = { 0xce, 0xbf, 0x37, 0x37, 0x12, 0x56 } };
	bt_addr_le_t a2 = { .type = 0x00,
			    .a.val = {0xc1, 0xcf, 0x2d, 0x70, 0x13, 0xa7 } };
	uint8_t exp[16] = { 0x61, 0x8f, 0x95, 0xda, 0x09, 0x0b, 0x6c, 0xd2,
			    0xc5, 0xe8, 0xd0, 0x9c, 0x98, 0x73, 0xc4, 0xe3 };
	uint8_t res[16];
	int err;

	err = bt_crypto_f6(w, n1, n2, r, io_cap, &a1, &a2, res);
	if (err)
		return err;

	if (memcmp(res, exp, 16))
		return -EINVAL;

	return 0;
}

static int smp_g2_test(void)
{
	uint8_t u[32] = { 0xe6, 0x9d, 0x35, 0x0e, 0x48, 0x01, 0x03, 0xcc,
			  0xdb, 0xfd, 0xf4, 0xac, 0x11, 0x91, 0xf4, 0xef,
			  0xb9, 0xa5, 0xf9, 0xe9, 0xa7, 0x83, 0x2c, 0x5e,
			  0x2c, 0xbe, 0x97, 0xf2, 0xd2, 0x03, 0xb0, 0x20 };
	uint8_t v[32] = { 0xfd, 0xc5, 0x7f, 0xf4, 0x49, 0xdd, 0x4f, 0x6b,
			  0xfb, 0x7c, 0x9d, 0xf1, 0xc2, 0x9a, 0xcb, 0x59,
			  0x2a, 0xe7, 0xd4, 0xee, 0xfb, 0xfc, 0x0a, 0x90,
			  0x9a, 0xbb, 0xf6, 0x32, 0x3d, 0x8b, 0x18, 0x55 };
	uint8_t x[16] = { 0xab, 0xae, 0x2b, 0x71, 0xec, 0xb2, 0xff, 0xff,
			  0x3e, 0x73, 0x77, 0xd1, 0x54, 0x84, 0xcb, 0xd5 };
	uint8_t y[16] = { 0xcf, 0xc4, 0x3d, 0xff, 0xf7, 0x83, 0x65, 0x21,
			  0x6e, 0x5f, 0xa7, 0x25, 0xcc, 0xe7, 0xe8, 0xa6 };
	uint32_t exp_val = 0x2f9ed5ba % 1000000;
	uint32_t val;
	int err;

	err = bt_crypto_g2(u, v, x, y, &val);
	if (err) {
		return err;
	}

	if (val != exp_val) {
		return -EINVAL;
	}

	return 0;
}

#if defined(CONFIG_BT_CLASSIC)
static int smp_h6_test(void)
{
	uint8_t w[16] = { 0x9b, 0x7d, 0x39, 0x0a, 0xa6, 0x10, 0x10, 0x34,
			  0x05, 0xad, 0xc8, 0x57, 0xa3, 0x34, 0x02, 0xec };
	uint8_t key_id[4] = { 0x72, 0x62, 0x65, 0x6c };
	uint8_t exp_res[16] = { 0x99, 0x63, 0xb1, 0x80, 0xe2, 0xa9, 0xd3, 0xe8,
				0x1c, 0xc9, 0x6d, 0xe7, 0x02, 0xe1, 0x9a, 0x2d};
	uint8_t res[16];
	int err;

	err = bt_crypto_h6(w, key_id, res);
	if (err) {
		return err;
	}

	if (memcmp(res, exp_res, 16)) {
		return -EINVAL;
	}

	return 0;
}

static int smp_h7_test(void)
{
	uint8_t salt[16] = { 0x31, 0x70, 0x6d, 0x74, 0x00, 0x00, 0x00, 0x00,
			     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	uint8_t w[16] = { 0x9b, 0x7d, 0x39, 0x0a, 0xa6, 0x10, 0x10, 0x34,
			  0x05, 0xad, 0xc8, 0x57, 0xa3, 0x34, 0x02, 0xec };
	uint8_t exp_res[16] = { 0x11, 0x70, 0xa5, 0x75, 0x2a, 0x8c, 0x99, 0xd2,
				0xec, 0xc0, 0xa3, 0xc6, 0x97, 0x35, 0x17, 0xfb};
	uint8_t res[16];
	int err;

	err = bt_crypto_h7(salt, w, res);
	if (err) {
		return err;
	}

	if (memcmp(res, exp_res, 16)) {
		return -EINVAL;
	}

	return 0;
}
#endif /* CONFIG_BT_CLASSIC */

static int smp_h8_test(void)
{
	uint8_t k[16] = {0xec, 0x02, 0x34, 0xa3, 0x57, 0xc8, 0xad, 0x05,
			 0x34, 0x10, 0x10, 0xa6, 0x0a, 0x39, 0x7d, 0x9b};
	uint8_t s[16] = {0x15, 0x36, 0xd1, 0x8d, 0xe3, 0xd2, 0x0d, 0xf9,
			 0x9b, 0x70, 0x44, 0xc1, 0x2f, 0x9e, 0xd5, 0xba};
	uint8_t key_id[4] = {0xcc, 0x03, 0x01, 0x48};

	uint8_t exp_res[16] = {0xe5, 0xe5, 0xbe, 0xba, 0xae, 0x72, 0x28, 0xe7,
			       0x22, 0xa3, 0x89, 0x04, 0xed, 0x35, 0x0f, 0x6d};
	uint8_t res[16];
	int err;

	err = bt_crypto_h8(k, s, key_id, res);
	if (err) {
		return err;
	}

	if (memcmp(res, exp_res, 16)) {
		return -EINVAL;
	}

	return 0;
}

static int smp_self_test(void)
{
	int err;

	err = smp_aes_cmac_test();
	if (err) {
		LOG_ERR("SMP AES-CMAC self tests failed");
		return err;
	}

	err = smp_sign_test();
	if (err) {
		LOG_ERR("SMP signing self tests failed");
		return err;
	}

	err = smp_f4_test();
	if (err) {
		LOG_ERR("SMP f4 self test failed");
		return err;
	}

	err = smp_f5_test();
	if (err) {
		LOG_ERR("SMP f5 self test failed");
		return err;
	}

	err = smp_f6_test();
	if (err) {
		LOG_ERR("SMP f6 self test failed");
		return err;
	}

	err = smp_g2_test();
	if (err) {
		LOG_ERR("SMP g2 self test failed");
		return err;
	}

#if defined(CONFIG_BT_CLASSIC)
	err = smp_h6_test();
	if (err) {
		LOG_ERR("SMP h6 self test failed");
		return err;
	}

	err = smp_h7_test();
	if (err) {
		LOG_ERR("SMP h7 self test failed");
		return err;
	}
#endif /* CONFIG_BT_CLASSIC */
	err = smp_h8_test();
	if (err) {
		LOG_ERR("SMP h8 self test failed");
		return err;
	}

	return 0;
}
#else
static inline int smp_self_test(void)
{
	return 0;
}
#endif

#if defined(CONFIG_BT_BONDABLE_PER_CONNECTION)
int bt_conn_set_bondable(struct bt_conn *conn, bool enable)
{
	struct bt_smp *smp;

	smp = smp_chan_get(conn);
	if (!smp) {
		return -EINVAL;
	}

	if (atomic_cas(&smp->bondable, BT_SMP_BONDABLE_UNINITIALIZED, (atomic_val_t)enable)) {
		return 0;
	} else {
		return -EALREADY;
	}
}
#endif

int bt_smp_auth_cb_overlay(struct bt_conn *conn, const struct bt_conn_auth_cb *cb)
{
	struct bt_smp *smp;

	smp = smp_chan_get(conn);
	if (!smp) {
		return -EINVAL;
	}

	if (atomic_ptr_cas(&smp->auth_cb, BT_SMP_AUTH_CB_UNINITIALIZED, (atomic_ptr_val_t)cb)) {
		return 0;
	} else {
		return -EALREADY;
	}
}

#if defined(CONFIG_BT_PASSKEY_KEYPRESS)
static int smp_send_keypress_notif(struct bt_smp *smp, uint8_t type)
{
	struct bt_smp_keypress_notif *req;
	struct net_buf *buf;

	buf = smp_create_pdu(smp, BT_SMP_KEYPRESS_NOTIFICATION, sizeof(*req));
	if (!buf) {
		return -ENOMEM;
	}

	req = net_buf_add(buf, sizeof(*req));
	req->type = type;

	smp_send(smp, buf, NULL, NULL);

	return 0;
}
#endif

#if defined(CONFIG_BT_PASSKEY_KEYPRESS)
int bt_smp_auth_keypress_notify(struct bt_conn *conn, enum bt_conn_auth_keypress type)
{
	struct bt_smp *smp;

	smp = smp_chan_get(conn);
	if (!smp) {
		return -EINVAL;
	}

	CHECKIF(!IN_RANGE(type,
			  BT_CONN_AUTH_KEYPRESS_ENTRY_STARTED,
			  BT_CONN_AUTH_KEYPRESS_ENTRY_COMPLETED)) {
		LOG_ERR("Refusing to send unknown event type %u", type);
		return -EINVAL;
	}

	if (smp->method != PASSKEY_INPUT ||
	    !atomic_test_bit(smp->flags, SMP_FLAG_USER)) {
		LOG_ERR("Refusing to send keypress: Not waiting for passkey input.");
		return -EINVAL;
	}

	return smp_send_keypress_notif(smp, type);
}
#endif

int bt_smp_auth_passkey_entry(struct bt_conn *conn, unsigned int passkey)
{
	struct bt_smp *smp;
	uint8_t err;

	smp = smp_chan_get(conn);
	if (!smp) {
		return -EINVAL;
	}

	if (!atomic_test_and_clear_bit(smp->flags, SMP_FLAG_USER)) {
		return -EINVAL;
	}

#if !defined(CONFIG_BT_SMP_SC_PAIR_ONLY)
	if (!atomic_test_bit(smp->flags, SMP_FLAG_SC)) {
		legacy_passkey_entry(smp, passkey);
		return 0;
	}
#endif /* !CONFIG_BT_SMP_SC_PAIR_ONLY */

	smp->passkey = sys_cpu_to_le32(passkey);

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    smp->chan.chan.conn->role == BT_HCI_ROLE_CENTRAL) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_CONFIRM);
		err = smp_send_pairing_confirm(smp);
		if (err) {
			smp_error(smp, BT_SMP_ERR_PASSKEY_ENTRY_FAILED);
			return 0;
		}
		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL) &&
	    atomic_test_bit(smp->flags, SMP_FLAG_CFM_DELAYED)) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_RANDOM);
		err = smp_send_pairing_confirm(smp);
		if (err) {
			smp_error(smp, BT_SMP_ERR_PASSKEY_ENTRY_FAILED);
			return 0;
		}
	}

	return 0;
}

int bt_smp_auth_passkey_confirm(struct bt_conn *conn)
{
	struct bt_smp *smp;

	smp = smp_chan_get(conn);
	if (!smp) {
		return -EINVAL;
	}

	if (!atomic_test_and_clear_bit(smp->flags, SMP_FLAG_USER)) {
		return -EINVAL;
	}

	/* wait for DHKey being generated */
	if (atomic_test_bit(smp->flags, SMP_FLAG_DHKEY_PENDING)) {
		atomic_set_bit(smp->flags, SMP_FLAG_DHKEY_SEND);
		return 0;
	}

	/* wait for remote DHKey Check */
	if (atomic_test_bit(smp->flags, SMP_FLAG_DHCHECK_WAIT)) {
		atomic_set_bit(smp->flags, SMP_FLAG_DHKEY_SEND);
		return 0;
	}

	if (atomic_test_bit(smp->flags, SMP_FLAG_DHKEY_SEND)) {
		uint8_t err;

#if defined(CONFIG_BT_CENTRAL)
		if (smp->chan.chan.conn->role == BT_HCI_ROLE_CENTRAL) {
			err = compute_and_send_central_dhcheck(smp);
			if (err) {
				smp_error(smp, err);
			}
			return 0;
		}
#endif /* CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_PERIPHERAL)
		err = compute_and_check_and_send_periph_dhcheck(smp);
		if (err) {
			smp_error(smp, err);
		}
#endif /* CONFIG_BT_PERIPHERAL */
	}

	return 0;
}

#if !defined(CONFIG_BT_SMP_SC_PAIR_ONLY)
int bt_smp_le_oob_set_tk(struct bt_conn *conn, const uint8_t *tk)
{
	struct bt_smp *smp;

	smp = smp_chan_get(conn);
	if (!smp || !tk) {
		return -EINVAL;
	}

	LOG_DBG("%s", bt_hex(tk, 16));

	if (!atomic_test_and_clear_bit(smp->flags, SMP_FLAG_USER)) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_LOG_SNIFFER_INFO)) {
		uint8_t oob[16];

		sys_memcpy_swap(oob, tk, 16);
		LOG_INF("Legacy OOB data 0x%s", bt_hex(oob, 16));
	}

	memcpy(smp->tk, tk, 16*sizeof(uint8_t));

	legacy_user_tk_entry(smp);

	return 0;
}
#endif /* !defined(CONFIG_BT_SMP_SC_PAIR_ONLY) */

int bt_smp_le_oob_generate_sc_data(struct bt_le_oob_sc_data *le_sc_oob)
{
	int err;

	if (!le_sc_supported()) {
		return -ENOTSUP;
	}

	if (!sc_public_key) {
		err = k_sem_take(&sc_local_pkey_ready, K_FOREVER);
		if (err) {
			return err;
		}
	}

	if (IS_ENABLED(CONFIG_BT_OOB_DATA_FIXED)) {
		uint8_t rand_num[] = {
			0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
			0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
		};

		memcpy(le_sc_oob->r, rand_num, sizeof(le_sc_oob->r));
	} else {
		err = bt_rand(le_sc_oob->r, 16);
		if (err) {
			return err;
		}
	}

	err = bt_crypto_f4(sc_public_key, sc_public_key, le_sc_oob->r, 0,
		     le_sc_oob->c);
	if (err) {
		return err;
	}

	return 0;
}

#if !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)
static bool le_sc_oob_data_check(struct bt_smp *smp, bool oobd_local_present,
				 bool oobd_remote_present)
{
	bool req_oob_present = le_sc_oob_data_req_check(smp);
	bool rsp_oob_present = le_sc_oob_data_rsp_check(smp);

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    smp->chan.chan.conn->role == BT_HCI_ROLE_CENTRAL) {
		if ((req_oob_present != oobd_remote_present) &&
		    (rsp_oob_present != oobd_local_present)) {
			return false;
		}
	} else if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		if ((req_oob_present != oobd_local_present) &&
		    (rsp_oob_present != oobd_remote_present)) {
			return false;
		}
	}

	return true;
}

static int le_sc_oob_pairing_continue(struct bt_smp *smp)
{
	if (smp->oobd_remote) {
		int err;
		uint8_t c[16];

		err = bt_crypto_f4(smp->pkey, smp->pkey, smp->oobd_remote->r, 0, c);
		if (err) {
			return err;
		}

		bool match = (memcmp(c, smp->oobd_remote->c, sizeof(c)) == 0);

		if (!match) {
			smp_error(smp, BT_SMP_ERR_CONFIRM_FAILED);
			return 0;
		}
	}

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    smp->chan.chan.conn->role == BT_HCI_ROLE_CENTRAL) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_RANDOM);
	} else if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_DHKEY_CHECK);
		atomic_set_bit(smp->flags, SMP_FLAG_DHCHECK_WAIT);
	}

	return smp_send_pairing_random(smp);
}

int bt_smp_le_oob_set_sc_data(struct bt_conn *conn,
			      const struct bt_le_oob_sc_data *oobd_local,
			      const struct bt_le_oob_sc_data *oobd_remote)
{
	struct bt_smp *smp;

	smp = smp_chan_get(conn);
	if (!smp) {
		return -EINVAL;
	}

	if (!le_sc_oob_data_check(smp, (oobd_local != NULL),
				  (oobd_remote != NULL))) {
		return -EINVAL;
	}

	if (!atomic_test_and_clear_bit(smp->flags, SMP_FLAG_OOB_PENDING)) {
		return -EINVAL;
	}

	smp->oobd_local = oobd_local;
	smp->oobd_remote = oobd_remote;

	return le_sc_oob_pairing_continue(smp);
}

int bt_smp_le_oob_get_sc_data(struct bt_conn *conn,
			      const struct bt_le_oob_sc_data **oobd_local,
			      const struct bt_le_oob_sc_data **oobd_remote)
{
	struct bt_smp *smp;

	smp = smp_chan_get(conn);
	if (!smp) {
		return -EINVAL;
	}

	if (!smp->oobd_local && !smp->oobd_remote) {
		return -ESRCH;
	}

	if (oobd_local) {
		*oobd_local = smp->oobd_local;
	}

	if (oobd_remote) {
		*oobd_remote = smp->oobd_remote;
	}

	return 0;
}
#endif /* !CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY */

int bt_smp_auth_cancel(struct bt_conn *conn)
{
	struct bt_smp *smp;

	smp = smp_chan_get(conn);
	if (!smp) {
		return -EINVAL;
	}

	if (!atomic_test_and_clear_bit(smp->flags, SMP_FLAG_USER)) {
		return -EINVAL;
	}

	LOG_DBG("");

	switch (smp->method) {
	case PASSKEY_INPUT:
	case PASSKEY_DISPLAY:
		return smp_error(smp, BT_SMP_ERR_PASSKEY_ENTRY_FAILED);
	case PASSKEY_CONFIRM:
		return smp_error(smp, BT_SMP_ERR_CONFIRM_FAILED);
	case LE_SC_OOB:
	case LEGACY_OOB:
		return smp_error(smp, BT_SMP_ERR_OOB_NOT_AVAIL);
	case JUST_WORKS:
		return smp_error(smp, BT_SMP_ERR_UNSPECIFIED);
	default:
		LOG_ERR("Unknown pairing method (%u)", smp->method);
		return 0;
	}
}

#if !defined(CONFIG_BT_SMP_SC_PAIR_ONLY)
int bt_smp_auth_pairing_confirm(struct bt_conn *conn)
{
	struct bt_smp *smp;

	smp = smp_chan_get(conn);
	if (!smp) {
		return -EINVAL;
	}

	if (!atomic_test_and_clear_bit(smp->flags, SMP_FLAG_USER)) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    conn->role == BT_CONN_ROLE_CENTRAL) {
		if (!atomic_test_bit(smp->flags, SMP_FLAG_SC)) {
			atomic_set_bit(smp->allowed_cmds,
				       BT_SMP_CMD_PAIRING_CONFIRM);
			return legacy_send_pairing_confirm(smp);
		}

		if (!sc_public_key) {
			atomic_set_bit(smp->flags, SMP_FLAG_PKEY_SEND);
			return 0;
		}

		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PUBLIC_KEY);
		return sc_send_public_key(smp);
	}

#if defined(CONFIG_BT_PERIPHERAL)
	if (!atomic_test_bit(smp->flags, SMP_FLAG_SC)) {
		atomic_set_bit(smp->allowed_cmds,
			       BT_SMP_CMD_PAIRING_CONFIRM);
		return send_pairing_rsp(smp);
	}

	atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PUBLIC_KEY);
	if (send_pairing_rsp(smp)) {
		return -EIO;
	}
#endif /* CONFIG_BT_PERIPHERAL */

	return 0;
}
#else
int bt_smp_auth_pairing_confirm(struct bt_conn *conn)
{
	/* confirm_pairing will never be called in LE SC only mode */
	return -EINVAL;
}
#endif /* !CONFIG_BT_SMP_SC_PAIR_ONLY */

#if defined(CONFIG_BT_FIXED_PASSKEY)
int bt_passkey_set(unsigned int passkey)
{
	if (passkey == BT_PASSKEY_INVALID) {
		fixed_passkey = BT_PASSKEY_INVALID;
		return 0;
	}

	if (passkey > 999999) {
		return -EINVAL;
	}

	fixed_passkey = passkey;
	return 0;
}
#endif /* CONFIG_BT_FIXED_PASSKEY */

int bt_smp_start_security(struct bt_conn *conn)
{
	switch (conn->role) {
#if defined(CONFIG_BT_CENTRAL)
	case BT_HCI_ROLE_CENTRAL:
	{
		int err;
		struct bt_smp *smp;

		smp = smp_chan_get(conn);
		if (!smp) {
			return -ENOTCONN;
		}

		/* pairing is in progress */
		if (atomic_test_bit(smp->flags, SMP_FLAG_PAIRING)) {
			return -EBUSY;
		}

		/* Encryption is in progress */
		if (atomic_test_bit(smp->flags, SMP_FLAG_ENC_PENDING)) {
			return -EBUSY;
		}

		if (!smp_keys_check(conn)) {
			return smp_send_pairing_req(conn);
		}

		/* LE SC LTK and legacy central LTK are stored in same place */
		err = bt_conn_le_start_encryption(conn,
						  conn->le.keys->ltk.rand,
						  conn->le.keys->ltk.ediv,
						  conn->le.keys->ltk.val,
						  conn->le.keys->enc_size);
		if (err) {
			return err;
		}

		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_SECURITY_REQUEST);
		atomic_set_bit(smp->flags, SMP_FLAG_ENC_PENDING);
		return 0;
	}
#endif /* CONFIG_BT_CENTRAL && CONFIG_BT_SMP */
#if defined(CONFIG_BT_PERIPHERAL)
	case BT_HCI_ROLE_PERIPHERAL:
		return smp_send_security_req(conn);
#endif /* CONFIG_BT_PERIPHERAL && CONFIG_BT_SMP */
	default:
		return -EINVAL;
	}
}

void bt_smp_update_keys(struct bt_conn *conn)
{
	struct bt_smp *smp;

	smp = smp_chan_get(conn);
	if (!smp) {
		return;
	}

	if (!atomic_test_bit(smp->flags, SMP_FLAG_PAIRING)) {
		return;
	}

	/*
	 * If link was successfully encrypted cleanup old keys as from now on
	 * only keys distributed in this pairing or LTK from LE SC will be used.
	 */
	if (conn->le.keys) {
		bt_keys_clear(conn->le.keys);
	}

	conn->le.keys = bt_keys_get_addr(conn->id, &conn->le.dst);
	if (!conn->le.keys) {
		LOG_ERR("Unable to get keys for %s", bt_addr_le_str(&conn->le.dst));
		smp_error(smp, BT_SMP_ERR_UNSPECIFIED);
		return;
	}

	/* mark keys as debug */
	if (atomic_test_bit(smp->flags, SMP_FLAG_SC_DEBUG_KEY)) {
		conn->le.keys->flags |= BT_KEYS_DEBUG;
	}

	/*
	 * store key type deducted from pairing method used
	 * it is important to store it since type is used to determine
	 * security level upon encryption
	 */
	switch (smp->method) {
	case LE_SC_OOB:
	case LEGACY_OOB:
		conn->le.keys->flags |= BT_KEYS_OOB;
		/* fallthrough */
	case PASSKEY_DISPLAY:
	case PASSKEY_INPUT:
	case PASSKEY_CONFIRM:
		conn->le.keys->flags |= BT_KEYS_AUTHENTICATED;
		break;
	case JUST_WORKS:
	default:
		/* unauthenticated key, clear it */
		conn->le.keys->flags &= ~BT_KEYS_OOB;
		conn->le.keys->flags &= ~BT_KEYS_AUTHENTICATED;
		break;
	}

	conn->le.keys->enc_size = get_encryption_key_size(smp);

	/*
	 * Store LTK if LE SC is used, this is safe since LE SC is mutually
	 * exclusive with legacy pairing. Other keys are added on keys
	 * distribution.
	 */
	if (atomic_test_bit(smp->flags, SMP_FLAG_SC)) {
		conn->le.keys->flags |= BT_KEYS_SC;

		if (atomic_test_bit(smp->flags, SMP_FLAG_BOND)) {
			bt_keys_add_type(conn->le.keys, BT_KEYS_LTK_P256);
			memcpy(conn->le.keys->ltk.val, smp->tk,
			       sizeof(conn->le.keys->ltk.val));
			(void)memset(conn->le.keys->ltk.rand, 0,
				     sizeof(conn->le.keys->ltk.rand));
			(void)memset(conn->le.keys->ltk.ediv, 0,
				     sizeof(conn->le.keys->ltk.ediv));
		} else if (IS_ENABLED(CONFIG_BT_LOG_SNIFFER_INFO)) {
			uint8_t ltk[16];

			sys_memcpy_swap(ltk, smp->tk, conn->le.keys->enc_size);
			LOG_INF("SC LTK: 0x%s (No bonding)", bt_hex(ltk, conn->le.keys->enc_size));
		}
	} else {
		conn->le.keys->flags &= ~BT_KEYS_SC;
	}
}

static int bt_smp_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
{
	int i;
	static const struct bt_l2cap_chan_ops ops = {
		.connected = bt_smp_connected,
		.disconnected = bt_smp_disconnected,
		.encrypt_change = bt_smp_encrypt_change,
		.recv = bt_smp_recv,
	};

	LOG_DBG("conn %p handle %u", conn, conn->handle);

	for (i = 0; i < ARRAY_SIZE(bt_smp_pool); i++) {
		struct bt_smp *smp = &bt_smp_pool[i];

		if (smp->chan.chan.conn) {
			continue;
		}

		smp->chan.chan.ops = &ops;

		*chan = &smp->chan.chan;

		return 0;
	}

	LOG_ERR("No available SMP context for conn %p", conn);

	return -ENOMEM;
}

BT_L2CAP_CHANNEL_DEFINE(smp_fixed_chan, BT_L2CAP_CID_SMP, bt_smp_accept, NULL);
#if defined(CONFIG_BT_CLASSIC)
BT_L2CAP_CHANNEL_DEFINE(smp_br_fixed_chan, BT_L2CAP_CID_BR_SMP,
			bt_smp_br_accept, NULL);
#endif /* CONFIG_BT_CLASSIC */

int bt_smp_init(void)
{
	static struct bt_pub_key_cb pub_key_cb = {
		.func           = bt_smp_pkey_ready,
	};

	sc_supported = le_sc_supported();
	if (IS_ENABLED(CONFIG_BT_SMP_SC_PAIR_ONLY) && !sc_supported) {
		LOG_ERR("SC Pair Only Mode selected but LE SC not supported");
		return -ENOENT;
	}

	if (IS_ENABLED(CONFIG_BT_SMP_USB_HCI_CTLR_WORKAROUND)) {
		LOG_WRN("BT_SMP_USB_HCI_CTLR_WORKAROUND is enabled, which "
			"exposes a security vulnerability!");
	}

	LOG_DBG("LE SC %s", sc_supported ? "enabled" : "disabled");

	if (!IS_ENABLED(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)) {
		bt_pub_key_gen(&pub_key_cb);
	}

	return smp_self_test();
}
