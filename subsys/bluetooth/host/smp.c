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

#include <zephyr.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <sys/atomic.h>
#include <sys/util.h>
#include <sys/byteorder.h>
#include <debug/stack.h>

#include <net/buf.h>
#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/buf.h>

#include <tinycrypt/constants.h>
#include <tinycrypt/aes.h>
#include <tinycrypt/utils.h>
#include <tinycrypt/cmac_mode.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_SMP)
#define LOG_MODULE_NAME bt_smp
#include "common/log.h"

#include "hci_core.h"
#include "ecc.h"
#include "keys.h"
#include "conn_internal.h"
#include "l2cap_internal.h"
#include "smp.h"

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

#if defined(CONFIG_BT_BREDR)
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

#if defined(CONFIG_BT_BREDR)

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

#endif /* CONFIG_BT_BREDR */

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
	SMP_FLAG_DHKEY_SEND,	/* if should generate and send DHKey Check */
	SMP_FLAG_USER,		/* if waiting for user input */
	SMP_FLAG_DISPLAY,       /* if display_passkey() callback was called */
	SMP_FLAG_OOB_PENDING,	/* if waiting for OOB data */
	SMP_FLAG_BOND,		/* if bonding */
	SMP_FLAG_SC_DEBUG_KEY,	/* if Secure Connection are using debug key */
	SMP_FLAG_SEC_REQ,	/* if Security Request was sent/received */
	SMP_FLAG_DHCHECK_WAIT,	/* if waiting for remote DHCheck (as slave) */
	SMP_FLAG_DERIVE_LK,	/* if Link Key should be derived */
	SMP_FLAG_BR_CONNECTED,	/* if BR/EDR channel is connected */
	SMP_FLAG_BR_PAIR,	/* if should start BR/EDR pairing */
	SMP_FLAG_CT2,		/* if should use H7 for keys derivation */

	/* Total number of flags - must be at the end */
	SMP_NUM_FLAGS,
};

/* SMP channel specific context */
struct bt_smp {
	/* The channel this context is associated with */
	struct bt_l2cap_le_chan		chan;

	/* Commands that remote is allowed to send */
	atomic_t			allowed_cmds;

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
	uint8_t				pkey[64];

	/* DHKey */
	uint8_t				dhkey[32];

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

	/* Delayed work for timeout handling */
	struct k_delayed_work		work;
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

static const uint8_t sc_debug_public_key[64] = {
	0xe6, 0x9d, 0x35, 0x0e, 0x48, 0x01, 0x03, 0xcc, 0xdb, 0xfd, 0xf4, 0xac,
	0x11, 0x91, 0xf4, 0xef, 0xb9, 0xa5, 0xf9, 0xe9, 0xa7, 0x83, 0x2c, 0x5e,
	0x2c, 0xbe, 0x97, 0xf2, 0xd2, 0x03, 0xb0, 0x20, 0x8b, 0xd2, 0x89, 0x15,
	0xd0, 0x8e, 0x1c, 0x74, 0x24, 0x30, 0xed, 0x8f, 0xc2, 0x45, 0x63, 0x76,
	0x5c, 0x15, 0x52, 0x5a, 0xbf, 0x9a, 0x32, 0x63, 0x6d, 0xeb, 0x2a, 0x65,
	0x49, 0x9c, 0x80, 0xdc
};

#if defined(CONFIG_BT_BREDR)
/* SMP over BR/EDR channel specific context */
struct bt_smp_br {
	/* The channel this context is associated with */
	struct bt_l2cap_br_chan	chan;

	/* Commands that remote is allowed to send */
	atomic_t		allowed_cmds;

	/* Flags for SMP state machine */
	ATOMIC_DEFINE(flags, SMP_NUM_FLAGS);

	/* Local key distribution */
	uint8_t			local_dist;

	/* Remote key distribution */
	uint8_t			remote_dist;

	/* Encryption Key Size used for connection */
	uint8_t 			enc_key_size;

	/* Delayed work for timeout handling */
	struct k_delayed_work 	work;
};

static struct bt_smp_br bt_smp_br_pool[CONFIG_BT_MAX_CONN];
#endif /* CONFIG_BT_BREDR */

static struct bt_smp bt_smp_pool[CONFIG_BT_MAX_CONN];
static bool bondable = IS_ENABLED(CONFIG_BT_BONDABLE);
static bool oobd_present;
static bool sc_supported;
static const uint8_t *sc_public_key;
static K_SEM_DEFINE(sc_local_pkey_ready, 0, 1);

static uint8_t get_io_capa(void)
{
	if (!bt_auth) {
		goto no_callbacks;
	}

	/* Passkey Confirmation is valid only for LE SC */
	if (bt_auth->passkey_display && bt_auth->passkey_entry &&
	    (bt_auth->passkey_confirm || !sc_supported)) {
		return BT_SMP_IO_KEYBOARD_DISPLAY;
	}

	/* DisplayYesNo is useful only for LE SC */
	if (sc_supported && bt_auth->passkey_display &&
	    bt_auth->passkey_confirm) {
		return BT_SMP_IO_DISPLAY_YESNO;
	}

	if (bt_auth->passkey_entry) {
		if (IS_ENABLED(CONFIG_BT_FIXED_PASSKEY) &&
		    fixed_passkey != BT_PASSKEY_INVALID) {
			return BT_SMP_IO_KEYBOARD_DISPLAY;
		} else {
			return BT_SMP_IO_KEYBOARD_ONLY;
		}
	}

	if (bt_auth->passkey_display) {
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

	if (conn->required_sec_level > BT_SECURITY_L2 &&
	    !(conn->le.keys->flags & BT_KEYS_AUTHENTICATED)) {
		return false;
	}

	if (conn->required_sec_level > BT_SECURITY_L3 &&
	    !(conn->le.keys->flags & BT_KEYS_AUTHENTICATED) &&
	    !(conn->le.keys->keys & BT_KEYS_LTK_P256) &&
	    !(conn->le.keys->enc_size == BT_SMP_MAX_ENC_KEY_SIZE)) {
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

	return gen_method_sc[remote_io][get_io_capa()];
#else
	return JUST_WORKS;
#endif
}

static enum bt_security_err auth_err_get(uint8_t smp_err)
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
	case BT_SMP_ERR_UNSPECIFIED:
	default:
		return BT_SECURITY_ERR_UNSPECIFIED;
	}
}

#if defined(CONFIG_BT_SMP_APP_PAIRING_ACCEPT)
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
#endif /* CONFIG_BT_SMP_APP_PAIRING_ACCEPT */

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

/* Cypher based Message Authentication Code (CMAC) with AES 128 bit
 *
 * Input    : key    ( 128-bit key )
 *          : in     ( message to be authenticated )
 *          : len    ( length of the message in octets )
 * Output   : out    ( message authentication code )
 */
static int bt_smp_aes_cmac(const uint8_t *key, const uint8_t *in, size_t len,
			   uint8_t *out)
{
	struct tc_aes_key_sched_struct sched;
	struct tc_cmac_struct state;

	if (tc_cmac_setup(&state, key, &sched) == TC_CRYPTO_FAIL) {
		return -EIO;
	}

	if (tc_cmac_update(&state, in, len) == TC_CRYPTO_FAIL) {
		return -EIO;
	}

	if (tc_cmac_final(out, &state) == TC_CRYPTO_FAIL) {
		return -EIO;
	}

	return 0;
}

static int smp_d1(const uint8_t *key, uint16_t d, uint16_t r, uint8_t res[16])
{
	int err;

	BT_DBG("key %s d %u r %u", bt_hex(key, 16), d, r);

	sys_put_le16(d, &res[0]);
	sys_put_le16(r, &res[2]);
	memset(&res[4], 0, 16 - 4);

	err = bt_encrypt_le(key, res, res);
	if (err) {
		return err;
	}

	BT_DBG("res %s", bt_hex(res, 16));
	return 0;
}

static int smp_f4(const uint8_t *u, const uint8_t *v, const uint8_t *x,
		  uint8_t z, uint8_t res[16])
{
	uint8_t xs[16];
	uint8_t m[65];
	int err;

	BT_DBG("u %s", bt_hex(u, 32));
	BT_DBG("v %s", bt_hex(v, 32));
	BT_DBG("x %s z 0x%x", bt_hex(x, 16), z);

	/*
	 * U, V and Z are concatenated and used as input m to the function
	 * AES-CMAC and X is used as the key k.
	 *
	 * Core Spec 4.2 Vol 3 Part H 2.2.5
	 *
	 * note:
	 * bt_smp_aes_cmac uses BE data and smp_f4 accept LE so we swap
	 */
	sys_memcpy_swap(m, u, 32);
	sys_memcpy_swap(m + 32, v, 32);
	m[64] = z;

	sys_memcpy_swap(xs, x, 16);

	err = bt_smp_aes_cmac(xs, m, sizeof(m), res);
	if (err) {
		return err;
	}

	sys_mem_swap(res, 16);

	BT_DBG("res %s", bt_hex(res, 16));

	return err;
}

static int smp_f5(const uint8_t *w, const uint8_t *n1, const uint8_t *n2,
		  const bt_addr_le_t *a1, const bt_addr_le_t *a2, uint8_t *mackey,
		  uint8_t *ltk)
{
	static const uint8_t salt[16] = { 0x6c, 0x88, 0x83, 0x91, 0xaa, 0xf5,
					  0xa5, 0x38, 0x60, 0x37, 0x0b, 0xdb,
					  0x5a, 0x60, 0x83, 0xbe };
	uint8_t m[53] = { 0x00, /* counter */
			  0x62, 0x74, 0x6c, 0x65, /* keyID */
			  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /*n1*/
			  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /*2*/
			  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* a1 */
			  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* a2 */
			  0x01, 0x00 /* length */ };
	uint8_t t[16], ws[32];
	int err;

	BT_DBG("w %s", bt_hex(w, 32));
	BT_DBG("n1 %s", bt_hex(n1, 16));
	BT_DBG("n2 %s", bt_hex(n2, 16));

	sys_memcpy_swap(ws, w, 32);

	err = bt_smp_aes_cmac(salt, ws, 32, t);
	if (err) {
		return err;
	}

	BT_DBG("t %s", bt_hex(t, 16));

	sys_memcpy_swap(m + 5, n1, 16);
	sys_memcpy_swap(m + 21, n2, 16);
	m[37] = a1->type;
	sys_memcpy_swap(m + 38, a1->a.val, 6);
	m[44] = a2->type;
	sys_memcpy_swap(m + 45, a2->a.val, 6);

	err = bt_smp_aes_cmac(t, m, sizeof(m), mackey);
	if (err) {
		return err;
	}

	BT_DBG("mackey %1s", bt_hex(mackey, 16));

	sys_mem_swap(mackey, 16);

	/* counter for ltk is 1 */
	m[0] = 0x01;

	err = bt_smp_aes_cmac(t, m, sizeof(m), ltk);
	if (err) {
		return err;
	}

	BT_DBG("ltk %s", bt_hex(ltk, 16));

	sys_mem_swap(ltk, 16);

	return 0;
}

static int smp_f6(const uint8_t *w, const uint8_t *n1, const uint8_t *n2,
		  const uint8_t *r, const uint8_t *iocap, const bt_addr_le_t *a1,
		  const bt_addr_le_t *a2, uint8_t *check)
{
	uint8_t ws[16];
	uint8_t m[65];
	int err;

	BT_DBG("w %s", bt_hex(w, 16));
	BT_DBG("n1 %s", bt_hex(n1, 16));
	BT_DBG("n2 %s", bt_hex(n2, 16));
	BT_DBG("r %s", bt_hex(r, 16));
	BT_DBG("io_cap %s", bt_hex(iocap, 3));
	BT_DBG("a1 %s", bt_hex(a1, 7));
	BT_DBG("a2 %s", bt_hex(a2, 7));

	sys_memcpy_swap(m, n1, 16);
	sys_memcpy_swap(m + 16, n2, 16);
	sys_memcpy_swap(m + 32, r, 16);
	sys_memcpy_swap(m + 48, iocap, 3);

	m[51] = a1->type;
	memcpy(m + 52, a1->a.val, 6);
	sys_memcpy_swap(m + 52, a1->a.val, 6);

	m[58] = a2->type;
	memcpy(m + 59, a2->a.val, 6);
	sys_memcpy_swap(m + 59, a2->a.val, 6);

	sys_memcpy_swap(ws, w, 16);

	err = bt_smp_aes_cmac(ws, m, sizeof(m), check);
	if (err) {
		return err;
	}

	BT_DBG("res %s", bt_hex(check, 16));

	sys_mem_swap(check, 16);

	return 0;
}

static int smp_g2(const uint8_t u[32], const uint8_t v[32],
		  const uint8_t x[16], const uint8_t y[16], uint32_t *passkey)
{
	uint8_t m[80], xs[16];
	int err;

	BT_DBG("u %s", bt_hex(u, 32));
	BT_DBG("v %s", bt_hex(v, 32));
	BT_DBG("x %s", bt_hex(x, 16));
	BT_DBG("y %s", bt_hex(y, 16));

	sys_memcpy_swap(m, u, 32);
	sys_memcpy_swap(m + 32, v, 32);
	sys_memcpy_swap(m + 64, y, 16);

	sys_memcpy_swap(xs, x, 16);

	/* reuse xs (key) as buffer for result */
	err = bt_smp_aes_cmac(xs, m, sizeof(m), xs);
	if (err) {
		return err;
	}
	BT_DBG("res %s", bt_hex(xs, 16));

	memcpy(passkey, xs + 12, 4);
	*passkey = sys_be32_to_cpu(*passkey) % 1000000;

	BT_DBG("passkey %u", *passkey);

	return 0;
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
static bool update_keys_check(struct bt_smp *smp)
{
	struct bt_conn *conn = smp->chan.chan.conn;

	if (!conn->le.keys) {
		conn->le.keys = bt_keys_get_addr(conn->id, &conn->le.dst);
	}

	if (IS_ENABLED(CONFIG_BT_SMP_DISABLE_LEGACY_JW_PASSKEY) &&
	    !atomic_test_bit(smp->flags, SMP_FLAG_SC) &&
	    smp->method != LEGACY_OOB) {
		return false;
	}

	if (IS_ENABLED(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY) &&
	    smp->method != LEGACY_OOB) {
		return false;
	}

	if (!conn->le.keys ||
	    !(conn->le.keys->keys & (BT_KEYS_LTK_P256 | BT_KEYS_LTK))) {
		return true;
	}

	if (conn->le.keys->enc_size > get_encryption_key_size(smp)) {
		return false;
	}

	if ((conn->le.keys->keys & BT_KEYS_LTK_P256) &&
	    !atomic_test_bit(smp->flags, SMP_FLAG_SC)) {
		return false;
	}

	if ((conn->le.keys->flags & BT_KEYS_AUTHENTICATED) &&
	     smp->method == JUST_WORKS) {
		return false;
	}

	if (!IS_ENABLED(CONFIG_BT_SMP_ALLOW_UNAUTH_OVERWRITE) &&
	    (!(conn->le.keys->flags & BT_KEYS_AUTHENTICATED)
	     && smp->method == JUST_WORKS)) {
		return false;
	}

	return true;
}

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
		return false;
	}

	return true;
}

#if defined(CONFIG_BT_PRIVACY) || defined(CONFIG_BT_SIGNING) || \
	!defined(CONFIG_BT_SMP_SC_PAIR_ONLY)
/* For TX callbacks */
static void smp_pairing_complete(struct bt_smp *smp, uint8_t status);
#if defined(CONFIG_BT_BREDR)
static void smp_pairing_br_complete(struct bt_smp_br *smp, uint8_t status);
#endif

static void smp_check_complete(struct bt_conn *conn, uint8_t dist_complete)
{
	struct bt_l2cap_chan *chan;

	if (conn->type == BT_CONN_TYPE_LE) {
		struct bt_smp *smp;

		chan = bt_l2cap_le_lookup_tx_cid(conn, BT_L2CAP_CID_SMP);
		__ASSERT(chan, "No SMP channel found");

		smp = CONTAINER_OF(chan, struct bt_smp, chan);
		smp->local_dist &= ~dist_complete;

		/* if all keys were distributed, pairing is done */
		if (!smp->local_dist && !smp->remote_dist) {
			smp_pairing_complete(smp, 0);
		}

		return;
	}

#if defined(CONFIG_BT_BREDR)
	if (conn->type == BT_CONN_TYPE_BR) {
		struct bt_smp_br *smp;

		chan = bt_l2cap_le_lookup_tx_cid(conn, BT_L2CAP_CID_BR_SMP);
		__ASSERT(chan, "No SMP channel found");

		smp = CONTAINER_OF(chan, struct bt_smp_br, chan);
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
static void smp_id_sent(struct bt_conn *conn, void *user_data)
{
	smp_check_complete(conn, BT_SMP_DIST_ID_KEY);
}
#endif /* CONFIG_BT_PRIVACY */

#if defined(CONFIG_BT_SIGNING)
static void smp_sign_info_sent(struct bt_conn *conn, void *user_data)
{
	smp_check_complete(conn, BT_SMP_DIST_SIGN);
}
#endif /* CONFIG_BT_SIGNING */

#if defined(CONFIG_BT_BREDR)
static int smp_h6(const uint8_t w[16], const uint8_t key_id[4], uint8_t res[16])
{
	uint8_t ws[16];
	uint8_t key_id_s[4];
	int err;

	BT_DBG("w %s", bt_hex(w, 16));
	BT_DBG("key_id %s", bt_hex(key_id, 4));

	sys_memcpy_swap(ws, w, 16);
	sys_memcpy_swap(key_id_s, key_id, 4);

	err = bt_smp_aes_cmac(ws, key_id_s, 4, res);
	if (err) {
		return err;
	}

	BT_DBG("res %s", bt_hex(res, 16));

	sys_mem_swap(res, 16);

	return 0;
}

static int smp_h7(const uint8_t salt[16], const uint8_t w[16], uint8_t res[16])
{
	uint8_t ws[16];
	uint8_t salt_s[16];
	int err;

	BT_DBG("w %s", bt_hex(w, 16));
	BT_DBG("salt %s", bt_hex(salt, 16));

	sys_memcpy_swap(ws, w, 16);
	sys_memcpy_swap(salt_s, salt, 16);

	err = bt_smp_aes_cmac(salt_s, ws, 16, res);
	if (err) {
		return err;
	}

	BT_DBG("res %s", bt_hex(res, 16));

	sys_mem_swap(res, 16);

	return 0;
}

static void sc_derive_link_key(struct bt_smp *smp)
{
	/* constants as specified in Core Spec Vol.3 Part H 2.4.2.4 */
	static const uint8_t lebr[4] = { 0x72, 0x62, 0x65, 0x6c };
	struct bt_conn *conn = smp->chan.chan.conn;
	struct bt_keys_link_key *link_key;
	uint8_t ilk[16];

	BT_DBG("");

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

		if (smp_h7(salt, conn->le.keys->ltk.val, ilk)) {
			bt_keys_link_key_clear(link_key);
			return;
		}
	} else {
		/* constants as specified in Core Spec Vol.3 Part H 2.4.2.4 */
		static const uint8_t tmp1[4] = { 0x31, 0x70, 0x6d, 0x74 };

		if (smp_h6(conn->le.keys->ltk.val, tmp1, ilk)) {
			bt_keys_link_key_clear(link_key);
			return;
		}
	}

	if (smp_h6(ilk, lebr, link_key->val)) {
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
	k_delayed_work_cancel(&smp->work);

	atomic_set(smp->flags, 0);
	atomic_set(&smp->allowed_cmds, 0);

	atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PAIRING_REQ);
}

static void smp_pairing_br_complete(struct bt_smp_br *smp, uint8_t status)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	struct bt_keys *keys;
	bt_addr_le_t addr;

	BT_DBG("status 0x%x", status);

	/* For dualmode devices LE address is same as BR/EDR address
	 * and is of public type.
	 */
	bt_addr_copy(&addr.a, &conn->br.dst);
	addr.type = BT_ADDR_LE_PUBLIC;
	keys = bt_keys_find_addr(conn->id, &addr);

	if (status) {
		if (keys) {
			bt_keys_clear(keys);
		}

		if (bt_auth && bt_auth->pairing_failed) {
			bt_auth->pairing_failed(smp->chan.chan.conn,
						auth_err_get(status));
		}
	} else {
		bool bond_flag = atomic_test_bit(smp->flags, SMP_FLAG_BOND);

		if (bond_flag && keys) {
			bt_keys_store(keys);
		}

		if (bt_auth && bt_auth->pairing_complete) {
			bt_auth->pairing_complete(smp->chan.chan.conn,
						  bond_flag);
		}
	}

	smp_br_reset(smp);
}

static void smp_br_timeout(struct k_work *work)
{
	struct bt_smp_br *smp = CONTAINER_OF(work, struct bt_smp_br, work);

	BT_ERR("SMP Timeout");

	smp_pairing_br_complete(smp, BT_SMP_ERR_UNSPECIFIED);
	atomic_set_bit(smp->flags, SMP_FLAG_TIMEOUT);
}

static void smp_br_send(struct bt_smp_br *smp, struct net_buf *buf,
			bt_conn_tx_cb_t cb)
{
	bt_l2cap_send_cb(smp->chan.chan.conn, BT_L2CAP_CID_BR_SMP, buf, cb,
			 NULL);
	k_delayed_work_submit(&smp->work, SMP_TIMEOUT);
}

static void bt_smp_br_connected(struct bt_l2cap_chan *chan)
{
	struct bt_smp_br *smp = CONTAINER_OF(chan, struct bt_smp_br, chan);

	BT_DBG("chan %p cid 0x%04x", chan,
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
	struct bt_smp_br *smp = CONTAINER_OF(chan, struct bt_smp_br, chan);

	BT_DBG("chan %p cid 0x%04x", chan,
	       CONTAINER_OF(chan, struct bt_l2cap_br_chan, chan)->tx.cid);

	k_delayed_work_cancel(&smp->work);

	(void)memset(smp, 0, sizeof(*smp));
}

static void smp_br_init(struct bt_smp_br *smp)
{
	/* Initialize SMP context without clearing L2CAP channel context */
	(void)memset((uint8_t *)smp + sizeof(smp->chan), 0,
		     sizeof(*smp) - (sizeof(smp->chan) + sizeof(smp->work)));

	atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PAIRING_FAIL);
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

	BT_DBG("");

	if (!link_key) {
		return;
	}

	if (IS_ENABLED(CONFIG_BT_SMP_FORCE_BREDR) && conn->encrypt != 0x02) {
		BT_WARN("Using P192 Link Key for P256 LTK derivation");
	}

	/*
	 * For dualmode devices LE address is same as BR/EDR address and is of
	 * public type.
	 */
	bt_addr_copy(&addr.a, &conn->br.dst);
	addr.type = BT_ADDR_LE_PUBLIC;

	keys = bt_keys_get_type(BT_KEYS_LTK_P256, conn->id, &addr);
	if (!keys) {
		BT_ERR("No keys space for %s", bt_addr_le_str(&addr));
		return;
	}

	if (atomic_test_bit(smp->flags, SMP_FLAG_CT2)) {
		/* constants as specified in Core Spec Vol.3 Part H 2.4.2.5 */
		static const uint8_t salt[16] = { 0x32, 0x70, 0x6d, 0x74,
					       0x00, 0x00, 0x00, 0x00,
					       0x00, 0x00, 0x00, 0x00,
					       0x00, 0x00, 0x00, 0x00 };

		if (smp_h7(salt, link_key->val, ilk)) {
			bt_keys_link_key_clear(link_key);
			return;
		}
	} else {
		/* constants as specified in Core Spec Vol.3 Part H 2.4.2.5 */
		static const uint8_t tmp2[4] = { 0x32, 0x70, 0x6d, 0x74 };

		if (smp_h6(link_key->val, tmp2, ilk)) {
			bt_keys_clear(keys);
			return;
		}
	}

	if (smp_h6(ilk, brle, keys->ltk.val)) {
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

	BT_DBG("LTK derived from LinkKey");
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
		BT_ERR("No keys space for %s", bt_addr_le_str(&addr));
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
			BT_ERR("Unable to allocate Ident Info buffer");
			return;
		}

		id_info = net_buf_add(buf, sizeof(*id_info));
		memcpy(id_info->irk, bt_dev.irk[conn->id], 16);

		smp_br_send(smp, buf, NULL);

		buf = smp_br_create_pdu(smp, BT_SMP_CMD_IDENT_ADDR_INFO,
				     sizeof(*id_addr_info));
		if (!buf) {
			BT_ERR("Unable to allocate Ident Addr Info buffer");
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
			BT_ERR("Unable to allocate Signing Info buffer");
			return;
		}

		info = net_buf_add(buf, sizeof(*info));

		bt_rand(info->csrk, sizeof(info->csrk));

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
		BT_WARN("Allowing BR/EDR SMP with P-192 key");
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

	BT_DBG("");

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

	smp_br_send(smp, rsp_buf, NULL);

	atomic_set_bit(smp->flags, SMP_FLAG_PAIRING);

	/* derive LTK if requested and clear distribution bits */
	if ((smp->local_dist & BT_SMP_DIST_ENC_KEY) &&
	    (smp->remote_dist & BT_SMP_DIST_ENC_KEY)) {
		smp_br_derive_ltk(smp);
	}
	smp->local_dist &= ~BT_SMP_DIST_ENC_KEY;
	smp->remote_dist &= ~BT_SMP_DIST_ENC_KEY;

	/* BR/EDR acceptor is like LE Slave and distributes keys first */
	smp_br_distribute_keys(smp);

	if (smp->remote_dist & BT_SMP_DIST_ID_KEY) {
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_IDENT_INFO);
	} else if (smp->remote_dist & BT_SMP_DIST_SIGN) {
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_SIGNING_INFO);
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

	BT_DBG("");

	max_key_size = bt_conn_enc_key_size(conn);
	if (!max_key_size) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	if (rsp->max_key_size != max_key_size) {
		return BT_SMP_ERR_ENC_KEY_SIZE;
	}

	smp->local_dist &= rsp->init_key_dist;
	smp->remote_dist &= rsp->resp_key_dist;

	smp->local_dist &= SEND_KEYS_SC;
	smp->remote_dist &= RECV_KEYS_SC;

	/* slave distributes its keys first */

	if (smp->remote_dist & BT_SMP_DIST_ID_KEY) {
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_IDENT_INFO);
	} else if (smp->remote_dist & BT_SMP_DIST_SIGN) {
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_SIGNING_INFO);
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

	BT_ERR("reason 0x%x", req->reason);

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

	BT_DBG("");

	/* TODO should we resolve LE address if matching RPA is connected? */

	/*
	 * For dualmode devices LE address is same as BR/EDR address and is of
	 * public type.
	 */
	bt_addr_copy(&addr.a, &conn->br.dst);
	addr.type = BT_ADDR_LE_PUBLIC;

	keys = bt_keys_get_type(BT_KEYS_IRK, conn->id, &addr);
	if (!keys) {
		BT_ERR("Unable to get keys for %s", bt_addr_le_str(&addr));
		return BT_SMP_ERR_UNSPECIFIED;
	}

	memcpy(keys->irk.val, req->irk, sizeof(keys->irk.val));

	atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_IDENT_ADDR_INFO);

	return 0;
}

static uint8_t smp_br_ident_addr_info(struct bt_smp_br *smp,
				      struct net_buf *buf)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	struct bt_smp_ident_addr_info *req = (void *)buf->data;
	bt_addr_le_t addr;

	BT_DBG("identity %s", bt_addr_le_str(&req->addr));

	/*
	 * For dual mode device identity address must be same as BR/EDR address
	 * and be of public type. So if received one doesn't match BR/EDR
	 * address we fail.
	 */

	bt_addr_copy(&addr.a, &conn->br.dst);
	addr.type = BT_ADDR_LE_PUBLIC;

	if (bt_addr_le_cmp(&addr, &req->addr)) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	smp->remote_dist &= ~BT_SMP_DIST_ID_KEY;

	if (smp->remote_dist & BT_SMP_DIST_SIGN) {
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_SIGNING_INFO);
	}

	if (conn->role == BT_CONN_ROLE_MASTER && !smp->remote_dist) {
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

	BT_DBG("");

	/*
	 * For dualmode devices LE address is same as BR/EDR address and is of
	 * public type.
	 */
	bt_addr_copy(&addr.a, &conn->br.dst);
	addr.type = BT_ADDR_LE_PUBLIC;

	keys = bt_keys_get_type(BT_KEYS_REMOTE_CSRK, conn->id, &addr);
	if (!keys) {
		BT_ERR("Unable to get keys for %s", bt_addr_le_str(&addr));
		return BT_SMP_ERR_UNSPECIFIED;
	}

	memcpy(keys->remote_csrk.val, req->csrk, sizeof(keys->remote_csrk.val));

	smp->remote_dist &= ~BT_SMP_DIST_SIGN;

	if (conn->role == BT_CONN_ROLE_MASTER && !smp->remote_dist) {
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
	{ }, /* master ident not used over BR/EDR */
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
	bt_l2cap_send(smp->chan.chan.conn, BT_L2CAP_CID_SMP, buf);

	return 0;
}

static int bt_smp_br_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_smp_br *smp = CONTAINER_OF(chan, struct bt_smp_br, chan);
	struct bt_smp_hdr *hdr;
	uint8_t err;

	if (buf->len < sizeof(*hdr)) {
		BT_ERR("Too small SMP PDU received");
		return 0;
	}

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	BT_DBG("Received SMP code 0x%02x len %u", hdr->code, buf->len);

	/*
	 * If SMP timeout occurred "no further SMP commands shall be sent over
	 * the L2CAP Security Manager Channel. A new SM procedure shall only be
	 * performed when a new physical link has been established."
	 */
	if (atomic_test_bit(smp->flags, SMP_FLAG_TIMEOUT)) {
		BT_WARN("SMP command (code 0x%02x) received after timeout",
			hdr->code);
		return 0;
	}

	if (hdr->code >= ARRAY_SIZE(br_handlers) ||
	    !br_handlers[hdr->code].func) {
		BT_WARN("Unhandled SMP code 0x%02x", hdr->code);
		smp_br_error(smp, BT_SMP_ERR_CMD_NOTSUPP);
		return 0;
	}

	if (!atomic_test_and_clear_bit(&smp->allowed_cmds, hdr->code)) {
		BT_WARN("Unexpected SMP code 0x%02x", hdr->code);
		smp_br_error(smp, BT_SMP_ERR_UNSPECIFIED);
		return 0;
	}

	if (buf->len != br_handlers[hdr->code].expect_len) {
		BT_ERR("Invalid len %u for code 0x%02x", buf->len, hdr->code);
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
		BT_WARN("Enabling BR/EDR SMP without BR/EDR SC support");
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

	BT_DBG("conn %p handle %u", conn, conn->handle);

	for (i = 0; i < ARRAY_SIZE(bt_smp_pool); i++) {
		struct bt_smp_br *smp = &bt_smp_br_pool[i];

		if (smp->chan.chan.conn) {
			continue;
		}

		smp->chan.chan.ops = &ops;

		*chan = &smp->chan.chan;

		k_delayed_work_init(&smp->work, smp_br_timeout);
		smp_br_reset(smp);

		return 0;
	}

	BT_ERR("No available SMP context for conn %p", conn);

	return -ENOMEM;
}

static struct bt_smp_br *smp_br_chan_get(struct bt_conn *conn)
{
	struct bt_l2cap_chan *chan;

	chan = bt_l2cap_br_lookup_rx_cid(conn, BT_L2CAP_CID_BR_SMP);
	if (!chan) {
		BT_ERR("Unable to find SMP channel");
		return NULL;
	}

	return CONTAINER_OF(chan, struct bt_smp_br, chan);
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

	atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PAIRING_RSP);

	atomic_set_bit(smp->flags, SMP_FLAG_PAIRING);

	return 0;
}
#endif /* CONFIG_BT_BREDR */

static void smp_reset(struct bt_smp *smp)
{
	struct bt_conn *conn = smp->chan.chan.conn;

	k_delayed_work_cancel(&smp->work);

	smp->method = JUST_WORKS;
	atomic_set(&smp->allowed_cmds, 0);
	atomic_set(smp->flags, 0);

	if (conn->required_sec_level != conn->sec_level) {
		/* TODO report error */
		/* reset required security level in case of error */
		conn->required_sec_level = conn->sec_level;
	}

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    conn->role == BT_HCI_ROLE_MASTER) {
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_SECURITY_REQUEST);
		return;
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PAIRING_REQ);
	}
}

/* Note: This function not only does set the status but also calls smp_reset
 * at the end which clears any flags previously set.
 */
static void smp_pairing_complete(struct bt_smp *smp, uint8_t status)
{
	BT_DBG("status 0x%x", status);

	if (!status) {
#if defined(CONFIG_BT_BREDR)
		/*
		 * Don't derive if Debug Keys are used.
		 * TODO should we allow this if BR/EDR is already connected?
		 */
		if (atomic_test_bit(smp->flags, SMP_FLAG_DERIVE_LK) &&
		    (!atomic_test_bit(smp->flags, SMP_FLAG_SC_DEBUG_KEY) ||
		     IS_ENABLED(CONFIG_BT_STORE_DEBUG_KEYS))) {
			sc_derive_link_key(smp);
		}
#endif /* CONFIG_BT_BREDR */
		bool bond_flag = atomic_test_bit(smp->flags, SMP_FLAG_BOND);

		if (bond_flag) {
			bt_keys_store(smp->chan.chan.conn->le.keys);
		}

		if (bt_auth && bt_auth->pairing_complete) {
			bt_auth->pairing_complete(smp->chan.chan.conn,
						  bond_flag);
		}
	} else {
		uint8_t auth_err = auth_err_get(status);

		/* Clear the key pool entry in case of pairing failure if the
		 * keys already existed before the pairing procedure or the
		 * pairing failed during key distribution.
		 */
		if (smp->chan.chan.conn->le.keys &&
		    (!smp->chan.chan.conn->le.keys->enc_size ||
		     atomic_test_bit(smp->flags, SMP_FLAG_KEYS_DISTR))) {
			bt_keys_clear(smp->chan.chan.conn->le.keys);
			smp->chan.chan.conn->le.keys = NULL;
		}

		if (!atomic_test_bit(smp->flags, SMP_FLAG_KEYS_DISTR)) {
			bt_conn_security_changed(smp->chan.chan.conn, status,
						 auth_err);
		}

		if (bt_auth && bt_auth->pairing_failed) {
			bt_auth->pairing_failed(smp->chan.chan.conn, auth_err);
		}
	}

	smp_reset(smp);
}

static void smp_timeout(struct k_work *work)
{
	struct bt_smp *smp = CONTAINER_OF(work, struct bt_smp, work);

	BT_ERR("SMP Timeout");

	smp_pairing_complete(smp, BT_SMP_ERR_UNSPECIFIED);

	/* smp_pairing_complete clears flags so setting timeout flag must come
	 * after it.
	 */
	atomic_set_bit(smp->flags, SMP_FLAG_TIMEOUT);
}

static void smp_send(struct bt_smp *smp, struct net_buf *buf,
		     bt_conn_tx_cb_t cb, void *user_data)
{
	bt_l2cap_send_cb(smp->chan.chan.conn, BT_L2CAP_CID_SMP, buf, cb, NULL);
	k_delayed_work_submit(&smp->work, SMP_TIMEOUT);
}

static int smp_error(struct bt_smp *smp, uint8_t reason)
{
	struct bt_smp_pairing_fail *rsp;
	struct net_buf *buf;

	/* reset context and report */
	smp_pairing_complete(smp, reason);

	buf = smp_create_pdu(smp, BT_SMP_CMD_PAIRING_FAIL, sizeof(*rsp));
	if (!buf) {
		return -ENOBUFS;
	}

	rsp = net_buf_add(buf, sizeof(*rsp));
	rsp->reason = reason;

	/* SMP timer is not restarted for PairingFailed so don't use smp_send */
	bt_l2cap_send(smp->chan.chan.conn, BT_L2CAP_CID_SMP, buf);

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
static void xor_128(const uint8_t p[16], const uint8_t q[16], uint8_t r[16])
{
	size_t len = 16;

	while (len--) {
		*r++ = *p++ ^ *q++;
	}
}

static int smp_c1(const uint8_t k[16], const uint8_t r[16],
		  const uint8_t preq[7], const uint8_t pres[7],
		  const bt_addr_le_t *ia, const bt_addr_le_t *ra,
		  uint8_t enc_data[16])
{
	uint8_t p1[16], p2[16];
	int err;

	BT_DBG("k %s", bt_hex(k, 16));
	BT_DBG("r %s", bt_hex(r, 16));
	BT_DBG("ia %s", bt_addr_le_str(ia));
	BT_DBG("ra %s", bt_addr_le_str(ra));
	BT_DBG("preq %s", bt_hex(preq, 7));
	BT_DBG("pres %s", bt_hex(pres, 7));

	/* pres, preq, rat and iat are concatenated to generate p1 */
	p1[0] = ia->type;
	p1[1] = ra->type;
	memcpy(p1 + 2, preq, 7);
	memcpy(p1 + 9, pres, 7);

	BT_DBG("p1 %s", bt_hex(p1, 16));

	/* c1 = e(k, e(k, r XOR p1) XOR p2) */

	/* Using enc_data as temporary output buffer */
	xor_128(r, p1, enc_data);

	err = bt_encrypt_le(k, enc_data, enc_data);
	if (err) {
		return err;
	}

	/* ra is concatenated with ia and padding to generate p2 */
	memcpy(p2, ra->a.val, 6);
	memcpy(p2 + 6, ia->a.val, 6);
	(void)memset(p2 + 12, 0, 4);

	BT_DBG("p2 %s", bt_hex(p2, 16));

	xor_128(enc_data, p2, enc_data);

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
		return BT_SMP_ERR_UNSPECIFIED;
	}

	buf = smp_create_pdu(smp, BT_SMP_CMD_PAIRING_CONFIRM, sizeof(*req));
	if (!buf) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	req = net_buf_add(buf, sizeof(*req));

	if (smp_f4(sc_public_key, smp->pkey, smp->prnd, r, req->val)) {
		net_buf_unref(buf);
		return BT_SMP_ERR_UNSPECIFIED;
	}

	smp_send(smp, buf, NULL, NULL);

	atomic_clear_bit(smp->flags, SMP_FLAG_CFM_DELAYED);

	return 0;
}

#if !defined(CONFIG_BT_SMP_SC_PAIR_ONLY)
static void smp_ident_sent(struct bt_conn *conn, void *user_data)
{
	smp_check_complete(conn, BT_SMP_DIST_ENC_KEY);
}

static void legacy_distribute_keys(struct bt_smp *smp)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	struct bt_keys *keys = conn->le.keys;

	if (smp->local_dist & BT_SMP_DIST_ENC_KEY) {
		struct bt_smp_encrypt_info *info;
		struct bt_smp_master_ident *ident;
		struct net_buf *buf;
		/* Use struct to get randomness in single call to bt_rand */
		struct {
			uint8_t key[16];
			uint8_t rand[8];
			uint8_t ediv[2];
		} rand;

		bt_rand((void *)&rand, sizeof(rand));

		buf = smp_create_pdu(smp, BT_SMP_CMD_ENCRYPT_INFO,
				     sizeof(*info));
		if (!buf) {
			BT_ERR("Unable to allocate Encrypt Info buffer");
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

		buf = smp_create_pdu(smp, BT_SMP_CMD_MASTER_IDENT,
				     sizeof(*ident));
		if (!buf) {
			BT_ERR("Unable to allocate Master Ident buffer");
			return;
		}

		ident = net_buf_add(buf, sizeof(*ident));
		memcpy(ident->rand, rand.rand, sizeof(ident->rand));
		memcpy(ident->ediv, rand.ediv, sizeof(ident->ediv));

		smp_send(smp, buf, smp_ident_sent, NULL);

		if (atomic_test_bit(smp->flags, SMP_FLAG_BOND)) {
			bt_keys_add_type(keys, BT_KEYS_SLAVE_LTK);

			memcpy(keys->slave_ltk.val, rand.key,
			       sizeof(keys->slave_ltk.val));
			memcpy(keys->slave_ltk.rand, rand.rand,
			       sizeof(keys->slave_ltk.rand));
			memcpy(keys->slave_ltk.ediv, rand.ediv,
			       sizeof(keys->slave_ltk.ediv));
		}
	}
}
#endif /* !CONFIG_BT_SMP_SC_PAIR_ONLY */

static uint8_t bt_smp_distribute_keys(struct bt_smp *smp)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	struct bt_keys *keys = conn->le.keys;

	if (!keys) {
		BT_ERR("No keys space for %s", bt_addr_le_str(&conn->le.dst));
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
			BT_ERR("Unable to allocate Ident Info buffer");
			return BT_SMP_ERR_UNSPECIFIED;
		}

		id_info = net_buf_add(buf, sizeof(*id_info));
		memcpy(id_info->irk, bt_dev.irk[conn->id], 16);

		smp_send(smp, buf, NULL, NULL);

		buf = smp_create_pdu(smp, BT_SMP_CMD_IDENT_ADDR_INFO,
				     sizeof(*id_addr_info));
		if (!buf) {
			BT_ERR("Unable to allocate Ident Addr Info buffer");
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
			BT_ERR("Unable to allocate Signing Info buffer");
			return BT_SMP_ERR_UNSPECIFIED;
		}

		info = net_buf_add(buf, sizeof(*info));

		bt_rand(info->csrk, sizeof(info->csrk));

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

static uint8_t smp_pairing_accept_query(struct bt_conn *conn,
				    struct bt_smp_pairing *pairing)
{
#if defined(CONFIG_BT_SMP_APP_PAIRING_ACCEPT)
	if (bt_auth && bt_auth->pairing_accept) {
		const struct bt_conn_pairing_feat feat = {
			.io_capability = pairing->io_capability,
			.oob_data_flag = pairing->oob_flag,
			.auth_req = pairing->auth_req,
			.max_enc_key_size = pairing->max_key_size,
			.init_key_dist = pairing->init_key_dist,
			.resp_key_dist = pairing->resp_key_dist
		};

		return smp_err_get(bt_auth->pairing_accept(conn, &feat));
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

	method = gen_method_legacy[remote_io][get_io_capa()];

	/* if both sides have KeyboardDisplay capabilities, initiator displays
	 * and responder inputs
	 */
	if (method == PASSKEY_ROLE) {
		if (smp->chan.chan.conn->role == BT_HCI_ROLE_MASTER) {
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
		BT_ERR("JustWorks failed, authenticated keys present");
		return BT_SMP_ERR_UNSPECIFIED;
	}

	switch (smp->method) {
	case LEGACY_OOB:
		if (bt_auth && bt_auth->oob_data_request) {
			struct bt_conn_oob_info info = {
				.type = BT_CONN_OOB_LE_LEGACY,
			};

			atomic_set_bit(smp->flags, SMP_FLAG_USER);
			bt_auth->oob_data_request(smp->chan.chan.conn, &info);
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

		if (bt_auth && bt_auth->passkey_display) {
			atomic_set_bit(smp->flags, SMP_FLAG_DISPLAY);
			bt_auth->passkey_display(conn, passkey);
		}

		sys_put_le32(passkey, smp->tk);

		break;
	case PASSKEY_INPUT:
		atomic_set_bit(smp->flags, SMP_FLAG_USER);
		bt_auth->passkey_entry(conn);
		break;
	case JUST_WORKS:
		break;
	default:
		BT_ERR("Unknown pairing method (%u)", smp->method);
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
	uint8_t ret;

	BT_DBG("");

	/* ask for consent if pairing is not due to sending SecReq*/
	if ((DISPLAY_FIXED(smp) || smp->method == JUST_WORKS) &&
	    !atomic_test_bit(smp->flags, SMP_FLAG_SEC_REQ) &&
	    bt_auth && bt_auth->pairing_confirm) {
		atomic_set_bit(smp->flags, SMP_FLAG_USER);
		bt_auth->pairing_confirm(smp->chan.chan.conn);
		return 0;
	}

	ret = send_pairing_rsp(smp);
	if (ret) {
		return ret;
	}

	atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PAIRING_CONFIRM);

	return legacy_request_tk(smp);
}
#endif /* CONFIG_BT_PERIPHERAL */

static uint8_t legacy_pairing_random(struct bt_smp *smp)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	uint8_t tmp[16];
	int err;

	BT_DBG("");

	/* calculate confirmation */
	err = smp_c1(smp->tk, smp->rrnd, smp->preq, smp->prsp,
		     &conn->le.init_addr, &conn->le.resp_addr, tmp);
	if (err) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	BT_DBG("pcnf %s", bt_hex(smp->pcnf, 16));
	BT_DBG("cfm %s", bt_hex(tmp, 16));

	if (memcmp(smp->pcnf, tmp, sizeof(smp->pcnf))) {
		return BT_SMP_ERR_CONFIRM_FAILED;
	}

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    conn->role == BT_HCI_ROLE_MASTER) {
		uint8_t ediv[2], rand[8];

		/* No need to store master STK */
		err = smp_s1(smp->tk, smp->rrnd, smp->prnd, tmp);
		if (err) {
			return BT_SMP_ERR_UNSPECIFIED;
		}

		/* Rand and EDiv are 0 for the STK */
		(void)memset(ediv, 0, sizeof(ediv));
		(void)memset(rand, 0, sizeof(rand));
		if (bt_conn_le_start_encryption(conn, rand, ediv, tmp,
						get_encryption_key_size(smp))) {
			BT_ERR("Failed to start encryption");
			return BT_SMP_ERR_UNSPECIFIED;
		}

		atomic_set_bit(smp->flags, SMP_FLAG_ENC_PENDING);

		if (IS_ENABLED(CONFIG_BT_SMP_USB_HCI_CTLR_WORKAROUND)) {
			if (smp->remote_dist & BT_SMP_DIST_ENC_KEY) {
				atomic_set_bit(&smp->allowed_cmds,
					       BT_SMP_CMD_ENCRYPT_INFO);
			} else if (smp->remote_dist & BT_SMP_DIST_ID_KEY) {
				atomic_set_bit(&smp->allowed_cmds,
					       BT_SMP_CMD_IDENT_INFO);
			} else if (smp->remote_dist & BT_SMP_DIST_SIGN) {
				atomic_set_bit(&smp->allowed_cmds,
					       BT_SMP_CMD_SIGNING_INFO);
			}
		}

		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		err = smp_s1(smp->tk, smp->prnd, smp->rrnd, tmp);
		if (err) {
			return BT_SMP_ERR_UNSPECIFIED;
		}

		memcpy(smp->tk, tmp, sizeof(smp->tk));
		BT_DBG("generated STK %s", bt_hex(smp->tk, 16));

		atomic_set_bit(smp->flags, SMP_FLAG_ENC_PENDING);

		return smp_send_pairing_random(smp);
	}

	return 0;
}

static uint8_t legacy_pairing_confirm(struct bt_smp *smp)
{
	BT_DBG("");

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    smp->chan.chan.conn->role == BT_HCI_ROLE_MASTER) {
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PAIRING_CONFIRM);
		return legacy_send_pairing_confirm(smp);
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		if (!atomic_test_bit(smp->flags, SMP_FLAG_USER)) {
			atomic_set_bit(&smp->allowed_cmds,
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
	    smp->chan.chan.conn->role == BT_HCI_ROLE_MASTER) {
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PAIRING_CONFIRM);
		return;
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PAIRING_RANDOM);
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
	BT_DBG("");

	if (atomic_test_bit(smp->flags, SMP_FLAG_BOND)) {
		struct bt_smp_encrypt_info *req = (void *)buf->data;
		struct bt_conn *conn = smp->chan.chan.conn;
		struct bt_keys *keys;

		keys = bt_keys_get_type(BT_KEYS_LTK, conn->id, &conn->le.dst);
		if (!keys) {
			BT_ERR("Unable to get keys for %s",
			       bt_addr_le_str(&conn->le.dst));
			return BT_SMP_ERR_UNSPECIFIED;
		}

		memcpy(keys->ltk.val, req->ltk, 16);
	}

	atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_MASTER_IDENT);

	return 0;
}

static uint8_t smp_master_ident(struct bt_smp *smp, struct net_buf *buf)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	uint8_t err;

	BT_DBG("");

	if (atomic_test_bit(smp->flags, SMP_FLAG_BOND)) {
		struct bt_smp_master_ident *req = (void *)buf->data;
		struct bt_keys *keys;

		keys = bt_keys_get_type(BT_KEYS_LTK, conn->id, &conn->le.dst);
		if (!keys) {
			BT_ERR("Unable to get keys for %s",
			       bt_addr_le_str(&conn->le.dst));
			return BT_SMP_ERR_UNSPECIFIED;
		}

		memcpy(keys->ltk.ediv, req->ediv, sizeof(keys->ltk.ediv));
		memcpy(keys->ltk.rand, req->rand, sizeof(req->rand));

		smp->remote_dist &= ~BT_SMP_DIST_ENC_KEY;
	}

	if (smp->remote_dist & BT_SMP_DIST_ID_KEY) {
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_IDENT_INFO);
	} else if (smp->remote_dist & BT_SMP_DIST_SIGN) {
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_SIGNING_INFO);
	}

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    conn->role == BT_HCI_ROLE_MASTER && !smp->remote_dist) {
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
	uint8_t ret;

	BT_DBG("");

	/* ask for consent if this is due to received SecReq */
	if ((DISPLAY_FIXED(smp) || smp->method == JUST_WORKS) &&
	    atomic_test_bit(smp->flags, SMP_FLAG_SEC_REQ) &&
	    bt_auth && bt_auth->pairing_confirm) {
		atomic_set_bit(smp->flags, SMP_FLAG_USER);
		bt_auth->pairing_confirm(smp->chan.chan.conn);
		return 0;
	}

	ret = legacy_request_tk(smp);
	if (ret) {
		return ret;
	}

	if (!atomic_test_bit(smp->flags, SMP_FLAG_USER)) {
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PAIRING_CONFIRM);
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

static uint8_t smp_master_ident(struct bt_smp *smp, struct net_buf *buf)
{
	return BT_SMP_ERR_CMD_NOTSUPP;
}
#endif /* !CONFIG_BT_SMP_SC_PAIR_ONLY */

static int smp_init(struct bt_smp *smp)
{
	/* Initialize SMP context without clearing L2CAP channel context */
	(void)memset((uint8_t *)smp + sizeof(smp->chan), 0,
		     sizeof(*smp) - (sizeof(smp->chan) + sizeof(smp->work)));

	/* Generate local random number */
	if (bt_rand(smp->prnd, 16)) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	BT_DBG("prnd %s", bt_hex(smp->prnd, 16));

	atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PAIRING_FAIL);

#if !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)
	sc_public_key = bt_pub_key_get();
#endif

	return 0;
}

void bt_set_bondable(bool enable)
{
	bondable = enable;
}

void bt_set_oob_data_flag(bool enable)
{
	oobd_present = enable;
}

static uint8_t get_auth(struct bt_conn *conn, uint8_t auth)
{
	if (sc_supported) {
		auth &= BT_SMP_AUTH_MASK_SC;
	} else {
		auth &= BT_SMP_AUTH_MASK;
	}

	if ((get_io_capa() == BT_SMP_IO_NO_INPUT_OUTPUT) ||
	    (!IS_ENABLED(CONFIG_BT_SMP_ENFORCE_MITM) &&
	    (conn->required_sec_level < BT_SECURITY_L3))) {
		auth &= ~(BT_SMP_AUTH_MITM);
	} else {
		auth |= BT_SMP_AUTH_MITM;
	}

	if (bondable) {
		auth |= BT_SMP_AUTH_BONDING;
	} else {
		auth &= ~BT_SMP_AUTH_BONDING;
	}

	return auth;
}

static bool sec_level_reachable(struct bt_conn *conn)
{
	switch (conn->required_sec_level) {
	case BT_SECURITY_L1:
	case BT_SECURITY_L2:
		return true;
	case BT_SECURITY_L3:
		return get_io_capa() != BT_SMP_IO_NO_INPUT_OUTPUT ||
		       (bt_auth && bt_auth->oob_data_request);
	case BT_SECURITY_L4:
		return (get_io_capa() != BT_SMP_IO_NO_INPUT_OUTPUT ||
			(bt_auth && bt_auth->oob_data_request)) && sc_supported;
	default:
		return false;
	}
}

static struct bt_smp *smp_chan_get(struct bt_conn *conn)
{
	struct bt_l2cap_chan *chan;

	chan = bt_l2cap_le_lookup_rx_cid(conn, BT_L2CAP_CID_SMP);
	if (!chan) {
		BT_ERR("Unable to find SMP channel");
		return NULL;
	}

	return CONTAINER_OF(chan, struct bt_smp, chan);
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
			conn->le.keys = bt_keys_find(BT_KEYS_SLAVE_LTK,
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

		return true;
	}

#if !defined(CONFIG_BT_SMP_SC_PAIR_ONLY)
	if (conn->le.keys && (conn->le.keys->keys & BT_KEYS_SLAVE_LTK) &&
	    !memcmp(conn->le.keys->slave_ltk.rand, &rand, 8) &&
	    !memcmp(conn->le.keys->slave_ltk.ediv, &ediv, 2)) {
		enc_size = conn->le.keys->enc_size;

		memcpy(ltk, conn->le.keys->slave_ltk.val, enc_size);
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
		 * initiated by slave.
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

	BT_DBG("");
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
	if (!(sec_level_reachable(conn) || smp_keys_check(conn))) {
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
	req->auth_req = get_auth(conn, BT_SMP_AUTH_DEFAULT);

	/* SMP timer is not restarted for SecRequest so don't use smp_send */
	bt_l2cap_send(conn, BT_L2CAP_CID_SMP, req_buf);

	atomic_set_bit(smp->flags, SMP_FLAG_SEC_REQ);
	atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PAIRING_REQ);

	return 0;
}

static uint8_t smp_pairing_req(struct bt_smp *smp, struct net_buf *buf)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	struct bt_smp_pairing *req = (void *)buf->data;
	struct bt_smp_pairing *rsp;

	BT_DBG("");

	if ((req->max_key_size > BT_SMP_MAX_ENC_KEY_SIZE) ||
	    (req->max_key_size < BT_SMP_MIN_ENC_KEY_SIZE)) {
		return BT_SMP_ERR_ENC_KEY_SIZE;
	}

	if (!conn->le.keys) {
		conn->le.keys = bt_keys_get_addr(conn->id, &conn->le.dst);
		if (!conn->le.keys) {
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

	rsp->auth_req = get_auth(conn, req->auth_req);
	rsp->io_capability = get_io_capa();
	rsp->oob_flag = oobd_present ? BT_SMP_OOB_PRESENT :
				       BT_SMP_OOB_NOT_PRESENT;
	rsp->max_key_size = BT_SMP_MAX_ENC_KEY_SIZE;
	rsp->init_key_dist = (req->init_key_dist & RECV_KEYS);
	rsp->resp_key_dist = (req->resp_key_dist & SEND_KEYS);

	if ((rsp->auth_req & BT_SMP_AUTH_SC) &&
	    (req->auth_req & BT_SMP_AUTH_SC)) {
		atomic_set_bit(smp->flags, SMP_FLAG_SC);

		rsp->init_key_dist &= RECV_KEYS_SC;
		rsp->resp_key_dist &= SEND_KEYS_SC;
	}

	if ((rsp->auth_req & BT_SMP_AUTH_CT2) &&
	    (req->auth_req & BT_SMP_AUTH_CT2)) {
		atomic_set_bit(smp->flags, SMP_FLAG_CT2);
	}

	smp->local_dist = rsp->resp_key_dist;
	smp->remote_dist = rsp->init_key_dist;

	if ((rsp->auth_req & BT_SMP_AUTH_BONDING) &&
	    (req->auth_req & BT_SMP_AUTH_BONDING)) {
		atomic_set_bit(smp->flags, SMP_FLAG_BOND);
	} else if (IS_ENABLED(CONFIG_BT_BONDING_REQUIRED)) {
		/* Reject pairing req if not both intend to bond */
		return BT_SMP_ERR_UNSPECIFIED;
	}

	atomic_set_bit(smp->flags, SMP_FLAG_PAIRING);

	smp->method = get_pair_method(smp, req->io_capability);

	if (!update_keys_check(smp)) {
		return BT_SMP_ERR_AUTH_REQUIREMENTS;
	}

	if (!atomic_test_bit(smp->flags, SMP_FLAG_SC)) {
#if defined(CONFIG_BT_SMP_SC_PAIR_ONLY)
		return BT_SMP_ERR_AUTH_REQUIREMENTS;
#else
		if (IS_ENABLED(CONFIG_BT_SMP_APP_PAIRING_ACCEPT)) {
			uint8_t err;

			err = smp_pairing_accept_query(smp->chan.chan.conn,
						      req);
			if (err) {
				return err;
			}
		}

		return legacy_pairing_req(smp);
#endif /* CONFIG_BT_SMP_SC_PAIR_ONLY */
	}

	if ((IS_ENABLED(CONFIG_BT_SMP_SC_ONLY) ||
	     conn->required_sec_level == BT_SECURITY_L4) &&
		smp->method == JUST_WORKS) {
		return BT_SMP_ERR_AUTH_REQUIREMENTS;
	}

	if ((IS_ENABLED(CONFIG_BT_SMP_SC_ONLY) ||
	     conn->required_sec_level == BT_SECURITY_L4) &&
	       get_encryption_key_size(smp) != BT_SMP_MAX_ENC_KEY_SIZE) {
		return BT_SMP_ERR_ENC_KEY_SIZE;
	}

	if (IS_ENABLED(CONFIG_BT_SMP_APP_PAIRING_ACCEPT)) {
		uint8_t err;

		err = smp_pairing_accept_query(smp->chan.chan.conn, req);
		if (err) {
			return err;
		}
	}

	if ((DISPLAY_FIXED(smp) || smp->method == JUST_WORKS) &&
	    !atomic_test_bit(smp->flags, SMP_FLAG_SEC_REQ) &&
	    bt_auth && bt_auth->pairing_confirm) {
		atomic_set_bit(smp->flags, SMP_FLAG_USER);
		bt_auth->pairing_confirm(smp->chan.chan.conn);
		return 0;
	}

	atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PUBLIC_KEY);
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

	BT_DBG("");

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

	/* Encryption is in progress */
	if (atomic_test_bit(smp->flags, SMP_FLAG_ENC_PENDING)) {
		return -EBUSY;
	}

	/* early verify if required sec level if reachable */
	if (!sec_level_reachable(conn)) {
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

	req->auth_req = get_auth(conn, BT_SMP_AUTH_DEFAULT);
	req->io_capability = get_io_capa();
	req->oob_flag = oobd_present ? BT_SMP_OOB_PRESENT :
				       BT_SMP_OOB_NOT_PRESENT;
	req->max_key_size = BT_SMP_MAX_ENC_KEY_SIZE;
	req->init_key_dist = SEND_KEYS;
	req->resp_key_dist = RECV_KEYS;

	smp->local_dist = SEND_KEYS;
	smp->remote_dist = RECV_KEYS;

	/* Store req for later use */
	smp->preq[0] = BT_SMP_CMD_PAIRING_REQ;
	memcpy(smp->preq + 1, req, sizeof(*req));

	smp_send(smp, req_buf, NULL, NULL);

	atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PAIRING_RSP);
	atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_SECURITY_REQUEST);
	atomic_set_bit(smp->flags, SMP_FLAG_PAIRING);

	return 0;
}

static uint8_t smp_pairing_rsp(struct bt_smp *smp, struct net_buf *buf)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	struct bt_smp_pairing *rsp = (void *)buf->data;
	struct bt_smp_pairing *req = (struct bt_smp_pairing *)&smp->preq[1];

	BT_DBG("");

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
		return BT_SMP_ERR_UNSPECIFIED;
	}

	smp->method = get_pair_method(smp, rsp->io_capability);

	if (!update_keys_check(smp)) {
		return BT_SMP_ERR_AUTH_REQUIREMENTS;
	}

	if (!atomic_test_bit(smp->flags, SMP_FLAG_SC)) {
#if defined(CONFIG_BT_SMP_SC_PAIR_ONLY)
		return BT_SMP_ERR_AUTH_REQUIREMENTS;
#else
		if (IS_ENABLED(CONFIG_BT_SMP_APP_PAIRING_ACCEPT)) {
			uint8_t err;

			err = smp_pairing_accept_query(smp->chan.chan.conn,
						       rsp);
			if (err) {
				return err;
			}
		}

		return legacy_pairing_rsp(smp);
#endif /* CONFIG_BT_SMP_SC_PAIR_ONLY */
	}

	if ((IS_ENABLED(CONFIG_BT_SMP_SC_ONLY) ||
	     conn->required_sec_level == BT_SECURITY_L4) &&
	     smp->method == JUST_WORKS) {
		return BT_SMP_ERR_AUTH_REQUIREMENTS;
	}

	if ((IS_ENABLED(CONFIG_BT_SMP_SC_ONLY) ||
	     conn->required_sec_level == BT_SECURITY_L4) &&
	     get_encryption_key_size(smp) != BT_SMP_MAX_ENC_KEY_SIZE) {
		return BT_SMP_ERR_ENC_KEY_SIZE;
	}

	smp->local_dist &= SEND_KEYS_SC;
	smp->remote_dist &= RECV_KEYS_SC;

	if (IS_ENABLED(CONFIG_BT_SMP_APP_PAIRING_ACCEPT)) {
		uint8_t err;

		err = smp_pairing_accept_query(smp->chan.chan.conn, rsp);
		if (err) {
			return err;
		}
	}

	if ((DISPLAY_FIXED(smp) || smp->method == JUST_WORKS) &&
	    atomic_test_bit(smp->flags, SMP_FLAG_SEC_REQ) &&
	    bt_auth && bt_auth->pairing_confirm) {
		atomic_set_bit(smp->flags, SMP_FLAG_USER);
		bt_auth->pairing_confirm(smp->chan.chan.conn);
		return 0;
	}

	if (!sc_public_key) {
		atomic_set_bit(smp->flags, SMP_FLAG_PKEY_SEND);
		return 0;
	}

	atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PUBLIC_KEY);
	atomic_clear_bit(&smp->allowed_cmds, BT_SMP_CMD_SECURITY_REQUEST);

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

	BT_DBG("");

	atomic_clear_bit(smp->flags, SMP_FLAG_DISPLAY);

	memcpy(smp->pcnf, req->val, sizeof(smp->pcnf));

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    smp->chan.chan.conn->role == BT_HCI_ROLE_MASTER) {
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PAIRING_RANDOM);
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
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PAIRING_RANDOM);
		return smp_send_pairing_confirm(smp);
	case PASSKEY_INPUT:
		if (atomic_test_bit(smp->flags, SMP_FLAG_USER)) {
			atomic_set_bit(smp->flags, SMP_FLAG_CFM_DELAYED);
			return 0;
		}

		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PAIRING_RANDOM);
		return smp_send_pairing_confirm(smp);
	case JUST_WORKS:
	case PASSKEY_CONFIRM:
	default:
		return BT_SMP_ERR_UNSPECIFIED;
	}
}

static uint8_t sc_smp_send_dhkey_check(struct bt_smp *smp, const uint8_t *e)
{
	struct bt_smp_dhkey_check *req;
	struct net_buf *buf;

	BT_DBG("");

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
static uint8_t compute_and_send_master_dhcheck(struct bt_smp *smp)
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
		return BT_SMP_ERR_UNSPECIFIED;
	}

	/* calculate LTK and mackey */
	if (smp_f5(smp->dhkey, smp->prnd, smp->rrnd,
		   &smp->chan.chan.conn->le.init_addr,
		   &smp->chan.chan.conn->le.resp_addr, smp->mackey,
		   smp->tk)) {
		return BT_SMP_ERR_UNSPECIFIED;
	}
	/* calculate local DHKey check */
	if (smp_f6(smp->mackey, smp->prnd, smp->rrnd, r, &smp->preq[1],
		   &smp->chan.chan.conn->le.init_addr,
		   &smp->chan.chan.conn->le.resp_addr, e)) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	atomic_set_bit(&smp->allowed_cmds, BT_SMP_DHKEY_CHECK);
	return sc_smp_send_dhkey_check(smp, e);
}
#endif /* CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_PERIPHERAL)
static uint8_t compute_and_check_and_send_slave_dhcheck(struct bt_smp *smp)
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
		return BT_SMP_ERR_UNSPECIFIED;
	}

	/* calculate LTK and mackey */
	if (smp_f5(smp->dhkey, smp->rrnd, smp->prnd,
		   &smp->chan.chan.conn->le.init_addr,
		   &smp->chan.chan.conn->le.resp_addr, smp->mackey,
		   smp->tk)) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	/* calculate local DHKey check */
	if (smp_f6(smp->mackey, smp->prnd, smp->rrnd, r, &smp->prsp[1],
		   &smp->chan.chan.conn->le.resp_addr,
		   &smp->chan.chan.conn->le.init_addr, e)) {
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
	if (smp_f6(smp->mackey, smp->rrnd, smp->prnd, r, &smp->preq[1],
		   &smp->chan.chan.conn->le.init_addr,
		   &smp->chan.chan.conn->le.resp_addr, re)) {
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

static void bt_smp_dhkey_ready(const uint8_t *dhkey)
{
	struct bt_smp *smp = NULL;
	int i;

	BT_DBG("%p", dhkey);

	for (i = 0; i < ARRAY_SIZE(bt_smp_pool); i++) {
		if (atomic_test_and_clear_bit(bt_smp_pool[i].flags,
					      SMP_FLAG_DHKEY_PENDING)) {
			smp = &bt_smp_pool[i];
			break;
		}
	}

	if (!smp) {
		return;
	}

	if (!dhkey) {
		smp_error(smp, BT_SMP_ERR_DHKEY_CHECK_FAILED);
		return;
	}

	memcpy(smp->dhkey, dhkey, 32);

	/* wait for user passkey confirmation */
	if (atomic_test_bit(smp->flags, SMP_FLAG_USER)) {
		atomic_set_bit(smp->flags, SMP_FLAG_DHKEY_SEND);
		return;
	}

	/* wait for remote DHKey Check */
	if (atomic_test_bit(smp->flags, SMP_FLAG_DHCHECK_WAIT)) {
		atomic_set_bit(smp->flags, SMP_FLAG_DHKEY_SEND);
		return;
	}

	if (atomic_test_bit(smp->flags, SMP_FLAG_DHKEY_SEND)) {
		uint8_t err;

#if defined(CONFIG_BT_CENTRAL)
		if (smp->chan.chan.conn->role == BT_HCI_ROLE_MASTER) {
			err = compute_and_send_master_dhcheck(smp);
			if (err) {
				smp_error(smp, err);
			}

			return;
		}
#endif /* CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_PERIPHERAL)
		err = compute_and_check_and_send_slave_dhcheck(smp);
		if (err) {
			smp_error(smp, err);
		}
#endif /* CONFIG_BT_PERIPHERAL */
	}
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
		return BT_SMP_ERR_UNSPECIFIED;
	}

	if (smp_f4(smp->pkey, sc_public_key, smp->rrnd, r, cfm)) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	BT_DBG("pcnf %s", bt_hex(smp->pcnf, 16));
	BT_DBG("cfm %s", bt_hex(cfm, 16));

	if (memcmp(smp->pcnf, cfm, 16)) {
		return BT_SMP_ERR_CONFIRM_FAILED;
	}

	return 0;
}

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
	    smp->chan.chan.conn->role == BT_HCI_ROLE_MASTER) {
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

static uint8_t smp_pairing_random(struct bt_smp *smp, struct net_buf *buf)
{
	struct bt_smp_pairing_random *req = (void *)buf->data;
	uint32_t passkey;
	uint8_t err;

	BT_DBG("");

	memcpy(smp->rrnd, req->val, sizeof(smp->rrnd));

#if !defined(CONFIG_BT_SMP_SC_PAIR_ONLY)
	if (!atomic_test_bit(smp->flags, SMP_FLAG_SC)) {
		return legacy_pairing_random(smp);
	}
#endif /* !CONFIG_BT_SMP_SC_PAIR_ONLY */

#if defined(CONFIG_BT_CENTRAL)
	if (smp->chan.chan.conn->role == BT_HCI_ROLE_MASTER) {
		err = sc_smp_check_confirm(smp);
		if (err) {
			return err;
		}

		switch (smp->method) {
		case PASSKEY_CONFIRM:
			/* compare passkey before calculating LTK */
			if (smp_g2(sc_public_key, smp->pkey, smp->prnd,
				   smp->rrnd, &passkey)) {
				return BT_SMP_ERR_UNSPECIFIED;
			}

			atomic_set_bit(smp->flags, SMP_FLAG_USER);
			atomic_set_bit(smp->flags, SMP_FLAG_DHKEY_SEND);
			bt_auth->passkey_confirm(smp->chan.chan.conn, passkey);
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

			atomic_set_bit(&smp->allowed_cmds,
				       BT_SMP_CMD_PAIRING_CONFIRM);
			return smp_send_pairing_confirm(smp);
		default:
			return BT_SMP_ERR_UNSPECIFIED;
		}

		/* wait for DHKey being generated */
		if (atomic_test_bit(smp->flags, SMP_FLAG_DHKEY_PENDING)) {
			atomic_set_bit(smp->flags, SMP_FLAG_DHKEY_SEND);
			return 0;
		}

		return compute_and_send_master_dhcheck(smp);
	}
#endif /* CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_PERIPHERAL)
	switch (smp->method) {
	case PASSKEY_CONFIRM:
		if (smp_g2(smp->pkey, sc_public_key, smp->rrnd, smp->prnd,
			   &passkey)) {
			return BT_SMP_ERR_UNSPECIFIED;
		}

		atomic_set_bit(smp->flags, SMP_FLAG_USER);
		bt_auth->passkey_confirm(smp->chan.chan.conn, passkey);
		break;
	case JUST_WORKS:
		break;
	case PASSKEY_DISPLAY:
	case PASSKEY_INPUT:
		err = sc_smp_check_confirm(smp);
		if (err) {
			return err;
		}

		atomic_set_bit(&smp->allowed_cmds,
			       BT_SMP_CMD_PAIRING_CONFIRM);
		err = smp_send_pairing_random(smp);
		if (err) {
			return err;
		}

		smp->passkey_round++;
		if (smp->passkey_round == 20U) {
			atomic_set_bit(&smp->allowed_cmds, BT_SMP_DHKEY_CHECK);
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

		if (bt_auth && bt_auth->oob_data_request) {
			struct bt_conn_oob_info info = {
				.type = BT_CONN_OOB_LE_SC,
				.lesc.oob_config = BT_CONN_OOB_NO_DATA,
			};

			le_sc_oob_config_set(smp, &info);

			smp->oobd_local = NULL;
			smp->oobd_remote = NULL;

			atomic_set_bit(smp->flags, SMP_FLAG_OOB_PENDING);
			bt_auth->oob_data_request(smp->chan.chan.conn, &info);

			return 0;
		} else {
			return BT_SMP_ERR_OOB_NOT_AVAIL;
		}
	default:
		return BT_SMP_ERR_UNSPECIFIED;
	}

	atomic_set_bit(&smp->allowed_cmds, BT_SMP_DHKEY_CHECK);
	atomic_set_bit(smp->flags, SMP_FLAG_DHCHECK_WAIT);
	return smp_send_pairing_random(smp);
#else
	return BT_SMP_ERR_PAIRING_NOTSUPP;
#endif /* CONFIG_BT_PERIPHERAL */
}

static uint8_t smp_pairing_failed(struct bt_smp *smp, struct net_buf *buf)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	struct bt_smp_pairing_fail *req = (void *)buf->data;

	BT_ERR("reason 0x%x", req->reason);

	if (atomic_test_and_clear_bit(smp->flags, SMP_FLAG_USER) ||
	    atomic_test_and_clear_bit(smp->flags, SMP_FLAG_DISPLAY)) {
		if (bt_auth && bt_auth->cancel) {
			bt_auth->cancel(conn);
		}
	}

	smp_pairing_complete(smp, req->reason);

	/* return no error to avoid sending Pairing Failed in response */
	return 0;
}

static uint8_t smp_ident_info(struct bt_smp *smp, struct net_buf *buf)
{
	BT_DBG("");

	if (atomic_test_bit(smp->flags, SMP_FLAG_BOND)) {
		struct bt_smp_ident_info *req = (void *)buf->data;
		struct bt_conn *conn = smp->chan.chan.conn;
		struct bt_keys *keys;

		keys = bt_keys_get_type(BT_KEYS_IRK, conn->id, &conn->le.dst);
		if (!keys) {
			BT_ERR("Unable to get keys for %s",
			       bt_addr_le_str(&conn->le.dst));
			return BT_SMP_ERR_UNSPECIFIED;
		}

		memcpy(keys->irk.val, req->irk, 16);
	}

	atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_IDENT_ADDR_INFO);

	return 0;
}

static uint8_t smp_ident_addr_info(struct bt_smp *smp, struct net_buf *buf)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	struct bt_smp_ident_addr_info *req = (void *)buf->data;
	uint8_t err;

	BT_DBG("identity %s", bt_addr_le_str(&req->addr));

	if (!bt_addr_le_is_identity(&req->addr)) {
		BT_ERR("Invalid identity %s", bt_addr_le_str(&req->addr));
		BT_ERR(" for %s", bt_addr_le_str(&conn->le.dst));
		return BT_SMP_ERR_INVALID_PARAMS;
	}

	if (atomic_test_bit(smp->flags, SMP_FLAG_BOND)) {
		const bt_addr_le_t *dst;
		struct bt_keys *keys;

		keys = bt_keys_get_type(BT_KEYS_IRK, conn->id, &conn->le.dst);
		if (!keys) {
			BT_ERR("Unable to get keys for %s",
			       bt_addr_le_str(&conn->le.dst));
			return BT_SMP_ERR_UNSPECIFIED;
		}

		/*
		 * We can't use conn->dst here as this might already contain
		 * identity address known from previous pairing. Since all keys
		 * are cleared on re-pairing we wouldn't store IRK distributed
		 * in new pairing.
		 */
		if (conn->role == BT_HCI_ROLE_MASTER) {
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
				bt_addr_le_copy(&keys->addr, &req->addr);
				bt_addr_le_copy(&conn->le.dst, &req->addr);

				bt_conn_identity_resolved(conn);
			}
		}

		bt_id_add(keys);
	}

	smp->remote_dist &= ~BT_SMP_DIST_ID_KEY;

	if (smp->remote_dist & BT_SMP_DIST_SIGN) {
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_SIGNING_INFO);
	}

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    conn->role == BT_HCI_ROLE_MASTER && !smp->remote_dist) {
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

	BT_DBG("");

	if (atomic_test_bit(smp->flags, SMP_FLAG_BOND)) {
		struct bt_smp_signing_info *req = (void *)buf->data;
		struct bt_keys *keys;

		keys = bt_keys_get_type(BT_KEYS_REMOTE_CSRK, conn->id,
					&conn->le.dst);
		if (!keys) {
			BT_ERR("Unable to get keys for %s",
			       bt_addr_le_str(&conn->le.dst));
			return BT_SMP_ERR_UNSPECIFIED;
		}

		memcpy(keys->remote_csrk.val, req->csrk,
		       sizeof(keys->remote_csrk.val));
	}

	smp->remote_dist &= ~BT_SMP_DIST_SIGN;

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    conn->role == BT_HCI_ROLE_MASTER && !smp->remote_dist) {
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

	BT_DBG("");

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
	    !(bondable && (auth & BT_SMP_AUTH_BONDING))) {
		/* Reject security req if not both intend to bond */
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
		if (get_io_capa() != BT_SMP_IO_NO_INPUT_OUTPUT) {
			BT_INFO("New auth requirements: 0x%x, repairing",
				auth);
			goto pair;
		}

		BT_WARN("Unsupported auth requirements: 0x%x, repairing",
			auth);
		goto pair;
	}

	/* if LE SC required and no p256 key present repair */
	if ((auth & BT_SMP_AUTH_SC) &&
	    !(conn->le.keys->keys & BT_KEYS_LTK_P256)) {
		BT_INFO("New auth requirements: 0x%x, repairing", auth);
		goto pair;
	}

	if (bt_conn_le_start_encryption(conn, conn->le.keys->ltk.rand,
					conn->le.keys->ltk.ediv,
					conn->le.keys->ltk.val,
					conn->le.keys->enc_size) < 0) {
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

static uint8_t generate_dhkey(struct bt_smp *smp)
{
	if (IS_ENABLED(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	if (bt_dh_key_gen(smp->pkey, bt_smp_dhkey_ready)) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	atomic_set_bit(smp->flags, SMP_FLAG_DHKEY_PENDING);
	return 0;
}

static uint8_t display_passkey(struct bt_smp *smp)
{
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

	if (bt_auth && bt_auth->passkey_display) {
		atomic_set_bit(smp->flags, SMP_FLAG_DISPLAY);
		bt_auth->passkey_display(smp->chan.chan.conn, smp->passkey);
	}

	smp->passkey = sys_cpu_to_le32(smp->passkey);

	return 0;
}

#if defined(CONFIG_BT_PERIPHERAL)
static uint8_t smp_public_key_slave(struct bt_smp *smp)
{
	uint8_t err;

	err = sc_send_public_key(smp);
	if (err) {
		return err;
	}

	switch (smp->method) {
	case PASSKEY_CONFIRM:
	case JUST_WORKS:
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PAIRING_RANDOM);

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

		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PAIRING_CONFIRM);
		break;
	case PASSKEY_INPUT:
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PAIRING_CONFIRM);
		atomic_set_bit(smp->flags, SMP_FLAG_USER);
		bt_auth->passkey_entry(smp->chan.chan.conn);
		break;
	case LE_SC_OOB:
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PAIRING_RANDOM);
		break;
	default:
		return BT_SMP_ERR_UNSPECIFIED;
	}

	return generate_dhkey(smp);
}
#endif /* CONFIG_BT_PERIPHERAL */

static uint8_t smp_public_key(struct bt_smp *smp, struct net_buf *buf)
{
	struct bt_smp_public_key *req = (void *)buf->data;
	uint8_t err;

	BT_DBG("");

	memcpy(smp->pkey, req->x, 32);
	memcpy(&smp->pkey[32], req->y, 32);

	/* mark key as debug if remote is using it */
	if (memcmp(smp->pkey, sc_debug_public_key, 64) == 0) {
		BT_INFO("Remote is using Debug Public key");
		atomic_set_bit(smp->flags, SMP_FLAG_SC_DEBUG_KEY);

		/* Don't allow a bond established without debug key to be
		 * updated using LTK generated from debug key.
		 */
		if (!update_debug_keys_check(smp)) {
			return BT_SMP_ERR_AUTH_REQUIREMENTS;
		}
	}

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    smp->chan.chan.conn->role == BT_HCI_ROLE_MASTER) {
		switch (smp->method) {
		case PASSKEY_CONFIRM:
		case JUST_WORKS:
			atomic_set_bit(&smp->allowed_cmds,
				       BT_SMP_CMD_PAIRING_CONFIRM);
			break;
		case PASSKEY_DISPLAY:
			err = display_passkey(smp);
			if (err) {
				return err;
			}

			atomic_set_bit(&smp->allowed_cmds,
				       BT_SMP_CMD_PAIRING_CONFIRM);

			err = smp_send_pairing_confirm(smp);
			if (err) {
				return err;
			}
			break;
		case PASSKEY_INPUT:
			atomic_set_bit(smp->flags, SMP_FLAG_USER);
			bt_auth->passkey_entry(smp->chan.chan.conn);
			break;
		case LE_SC_OOB:
			/* Step 6: Select random N */
			if (bt_rand(smp->prnd, 16)) {
				return BT_SMP_ERR_UNSPECIFIED;
			}

			if (bt_auth && bt_auth->oob_data_request) {
				struct bt_conn_oob_info info = {
					.type = BT_CONN_OOB_LE_SC,
					.lesc.oob_config = BT_CONN_OOB_NO_DATA,
				};

				le_sc_oob_config_set(smp, &info);

				smp->oobd_local = NULL;
				smp->oobd_remote = NULL;

				atomic_set_bit(smp->flags,
					       SMP_FLAG_OOB_PENDING);
				bt_auth->oob_data_request(smp->chan.chan.conn,
							  &info);
			} else {
				return BT_SMP_ERR_OOB_NOT_AVAIL;
			}
			break;
		default:
			return BT_SMP_ERR_UNSPECIFIED;
		}

		return generate_dhkey(smp);
	}

#if defined(CONFIG_BT_PERIPHERAL)
	if (!sc_public_key) {
		atomic_set_bit(smp->flags, SMP_FLAG_PKEY_SEND);
		return 0;
	}

	err = smp_public_key_slave(smp);
	if (err) {
		return err;
	}
#endif /* CONFIG_BT_PERIPHERAL */

	return 0;
}

static uint8_t smp_dhkey_check(struct bt_smp *smp, struct net_buf *buf)
{
	struct bt_smp_dhkey_check *req = (void *)buf->data;

	BT_DBG("");

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    smp->chan.chan.conn->role == BT_HCI_ROLE_MASTER) {
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
			return BT_SMP_ERR_UNSPECIFIED;
		}

		/* calculate remote DHKey check for comparison */
		if (smp_f6(smp->mackey, smp->rrnd, smp->prnd, r, &smp->prsp[1],
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
			return BT_SMP_ERR_UNSPECIFIED;
		}

		atomic_set_bit(smp->flags, SMP_FLAG_ENC_PENDING);

		if (IS_ENABLED(CONFIG_BT_SMP_USB_HCI_CTLR_WORKAROUND)) {
			if (smp->remote_dist & BT_SMP_DIST_ID_KEY) {
				atomic_set_bit(&smp->allowed_cmds,
					       BT_SMP_CMD_IDENT_INFO);
			} else if (smp->remote_dist & BT_SMP_DIST_SIGN) {
				atomic_set_bit(&smp->allowed_cmds,
					       BT_SMP_CMD_SIGNING_INFO);
			}
		}

		return 0;
	}

#if defined(CONFIG_BT_PERIPHERAL)
	if (smp->chan.chan.conn->role == BT_HCI_ROLE_SLAVE) {
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

		return compute_and_check_and_send_slave_dhcheck(smp);
	}
#endif /* CONFIG_BT_PERIPHERAL */

	return 0;
}

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
	{ smp_master_ident,        sizeof(struct bt_smp_master_ident) },
	{ smp_ident_info,          sizeof(struct bt_smp_ident_info) },
	{ smp_ident_addr_info,     sizeof(struct bt_smp_ident_addr_info) },
	{ smp_signing_info,        sizeof(struct bt_smp_signing_info) },
	{ smp_security_request,    sizeof(struct bt_smp_security_request) },
	{ smp_public_key,          sizeof(struct bt_smp_public_key) },
	{ smp_dhkey_check,         sizeof(struct bt_smp_dhkey_check) },
};

static int bt_smp_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_smp *smp = CONTAINER_OF(chan, struct bt_smp, chan);
	struct bt_smp_hdr *hdr;
	uint8_t err;

	if (buf->len < sizeof(*hdr)) {
		BT_ERR("Too small SMP PDU received");
		return 0;
	}

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	BT_DBG("Received SMP code 0x%02x len %u", hdr->code, buf->len);

	/*
	 * If SMP timeout occurred "no further SMP commands shall be sent over
	 * the L2CAP Security Manager Channel. A new SM procedure shall only be
	 * performed when a new physical link has been established."
	 */
	if (atomic_test_bit(smp->flags, SMP_FLAG_TIMEOUT)) {
		BT_WARN("SMP command (code 0x%02x) received after timeout",
			hdr->code);
		return 0;
	}

	if (hdr->code >= ARRAY_SIZE(handlers) || !handlers[hdr->code].func) {
		BT_WARN("Unhandled SMP code 0x%02x", hdr->code);
		smp_error(smp, BT_SMP_ERR_CMD_NOTSUPP);
		return 0;
	}

	if (!atomic_test_and_clear_bit(&smp->allowed_cmds, hdr->code)) {
		BT_WARN("Unexpected SMP code 0x%02x", hdr->code);
		/* Don't send error responses to error PDUs */
		if (hdr->code != BT_SMP_CMD_PAIRING_FAIL) {
			smp_error(smp, BT_SMP_ERR_UNSPECIFIED);
		}
		return 0;
	}

	if (buf->len != handlers[hdr->code].expect_len) {
		BT_ERR("Invalid len %u for code 0x%02x", buf->len, hdr->code);
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

	BT_DBG("");

	sc_public_key = pkey;

	if (!pkey) {
		BT_WARN("Public key not available");
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
		    smp->chan.chan.conn->role == BT_HCI_ROLE_MASTER) {
			err = sc_send_public_key(smp);
			if (err) {
				smp_error(smp, err);
			}

			atomic_set_bit(&smp->allowed_cmds,
				       BT_SMP_CMD_PUBLIC_KEY);
			continue;
		}

#if defined(CONFIG_BT_PERIPHERAL)
		err = smp_public_key_slave(smp);
		if (err) {
			smp_error(smp, err);
		}
#endif /* CONFIG_BT_PERIPHERAL */
	}
}

static void bt_smp_connected(struct bt_l2cap_chan *chan)
{
	struct bt_smp *smp = CONTAINER_OF(chan, struct bt_smp, chan);

	BT_DBG("chan %p cid 0x%04x", chan,
	       CONTAINER_OF(chan, struct bt_l2cap_le_chan, chan)->tx.cid);

	k_delayed_work_init(&smp->work, smp_timeout);
	smp_reset(smp);
}

static void bt_smp_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_smp *smp = CONTAINER_OF(chan, struct bt_smp, chan);
	struct bt_keys *keys = chan->conn->le.keys;

	BT_DBG("chan %p cid 0x%04x", chan,
	       CONTAINER_OF(chan, struct bt_l2cap_le_chan, chan)->tx.cid);

	k_delayed_work_cancel(&smp->work);

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
	struct bt_smp *smp = CONTAINER_OF(chan, struct bt_smp, chan);
	struct bt_conn *conn = chan->conn;

	BT_DBG("chan %p conn %p handle %u encrypt 0x%02x hci status 0x%02x",
	       chan, conn, conn->handle, conn->encrypt, hci_status);

	atomic_clear_bit(smp->flags, SMP_FLAG_ENC_PENDING);

	if (hci_status) {
		return;
	}

	if (!conn->encrypt) {
		return;
	}

	/* We were waiting for encryption but with no pairing in progress.
	 * This can happen if paired slave sent Security Request and we
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
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_ENCRYPT_INFO);
	} else if (smp->remote_dist & BT_SMP_DIST_ID_KEY) {
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_IDENT_INFO);
	} else if (smp->remote_dist & BT_SMP_DIST_SIGN) {
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_SIGNING_INFO);
	}

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    IS_ENABLED(CONFIG_BT_PRIVACY) &&
	    !(smp->remote_dist & BT_SMP_DIST_ID_KEY)) {
		/* To resolve directed advertising we need our local IRK
		 * in the controllers resolving list, add it now since the
		 * peer has no identity key.
		 */
		bt_id_add(conn->le.keys);
	}

	atomic_set_bit(smp->flags, SMP_FLAG_KEYS_DISTR);

	/* Slave distributes it's keys first */
	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    conn->role == BT_HCI_ROLE_MASTER && smp->remote_dist) {
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

	BT_DBG("Signing msg %s len %u key %s", bt_hex(msg, len), len,
	       bt_hex(key, 16));

	sys_mem_swap(m, len + sizeof(cnt));
	sys_memcpy_swap(key_s, key, 16);

	err = bt_smp_aes_cmac(key_s, m, len + sizeof(cnt), tmp);
	if (err) {
		BT_ERR("Data signing failed");
		return err;
	}

	sys_mem_swap(tmp, sizeof(tmp));
	memcpy(tmp + 4, &cnt, sizeof(cnt));

	/* Swap original message back */
	sys_mem_swap(m, len + sizeof(cnt));

	memcpy(sig, tmp + 4, 12);

	BT_DBG("sig %s", bt_hex(sig, 12));

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
		BT_ERR("Unable to find Remote CSRK for %s",
		       bt_addr_le_str(&conn->le.dst));
		return -ENOENT;
	}

	/* Copy signing count */
	cnt = sys_cpu_to_le32(keys->remote_csrk.cnt);
	memcpy(net_buf_tail(buf) - sizeof(sig), &cnt, sizeof(cnt));

	BT_DBG("Sign data len %zu key %s count %u", buf->len - sizeof(sig),
	       bt_hex(keys->remote_csrk.val, 16), keys->remote_csrk.cnt);

	err = smp_sign_buf(keys->remote_csrk.val, buf->data,
			   buf->len - sizeof(sig));
	if (err) {
		BT_ERR("Unable to create signature for %s",
		       bt_addr_le_str(&conn->le.dst));
		return -EIO;
	};

	if (memcmp(sig, net_buf_tail(buf) - sizeof(sig), sizeof(sig))) {
		BT_ERR("Unable to verify signature for %s",
		       bt_addr_le_str(&conn->le.dst));
		return -EBADMSG;
	};

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
		BT_ERR("Unable to find local CSRK for %s",
		       bt_addr_le_str(&conn->le.dst));
		return -ENOENT;
	}

	/* Reserve space for data signature */
	net_buf_add(buf, 12);

	/* Copy signing count */
	cnt = sys_cpu_to_le32(keys->local_csrk.cnt);
	memcpy(net_buf_tail(buf) - 12, &cnt, sizeof(cnt));

	BT_DBG("Sign data len %u key %s count %u", buf->len,
	       bt_hex(keys->local_csrk.val, 16), keys->local_csrk.cnt);

	err = smp_sign_buf(keys->local_csrk.val, buf->data, buf->len - 12);
	if (err) {
		BT_ERR("Unable to create signature for %s",
		       bt_addr_le_str(&conn->le.dst));
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

static int aes_test(const char *prefix, const uint8_t *key, const uint8_t *m,
		    uint16_t len, const uint8_t *mac)
{
	uint8_t out[16];

	BT_DBG("%s: AES CMAC of message with len %u", prefix, len);

	bt_smp_aes_cmac(key, m, len, out);
	if (!memcmp(out, mac, 16)) {
		BT_DBG("%s: Success", prefix);
	} else {
		BT_ERR("%s: Failed", prefix);
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

static int sign_test(const char *prefix, const uint8_t *key, const uint8_t *m,
		     uint16_t len, const uint8_t *sig)
{
	uint8_t msg[len + sizeof(uint32_t) + 8];
	uint8_t orig[len + sizeof(uint32_t) + 8];
	uint8_t *out = msg + len;
	int err;

	BT_DBG("%s: Sign message with len %u", prefix, len);

	(void)memset(msg, 0, sizeof(msg));
	memcpy(msg, m, len);
	(void)memset(msg + len, 0, sizeof(uint32_t));

	memcpy(orig, msg, sizeof(msg));

	err = smp_sign_buf(key, msg, len);
	if (err) {
		return err;
	}

	/* Check original message */
	if (!memcmp(msg, orig, len + sizeof(uint32_t))) {
		BT_DBG("%s: Original message intact", prefix);
	} else {
		BT_ERR("%s: Original message modified", prefix);
		BT_DBG("%s: orig %s", prefix, bt_hex(orig, sizeof(orig)));
		BT_DBG("%s: msg %s", prefix, bt_hex(msg, sizeof(msg)));
		return -1;
	}

	if (!memcmp(out, sig, 12)) {
		BT_DBG("%s: Success", prefix);
	} else {
		BT_ERR("%s: Failed", prefix);
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

	err = smp_f4(u, v, x, z, res);
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

	err = smp_f5(w, n1, n2, &a1, &a2, mackey, ltk);
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

	err = smp_f6(w, n1, n2, r, io_cap, &a1, &a2, res);
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

	err = smp_g2(u, v, x, y, &val);
	if (err) {
		return err;
	}

	if (val != exp_val) {
		return -EINVAL;
	}

	return 0;
}

#if defined(CONFIG_BT_BREDR)
static int smp_h6_test(void)
{
	uint8_t w[16] = { 0x9b, 0x7d, 0x39, 0x0a, 0xa6, 0x10, 0x10, 0x34,
			  0x05, 0xad, 0xc8, 0x57, 0xa3, 0x34, 0x02, 0xec };
	uint8_t key_id[4] = { 0x72, 0x62, 0x65, 0x6c };
	uint8_t exp_res[16] = { 0x99, 0x63, 0xb1, 0x80, 0xe2, 0xa9, 0xd3, 0xe8,
				0x1c, 0xc9, 0x6d, 0xe7, 0x02, 0xe1, 0x9a, 0x2d};
	uint8_t res[16];
	int err;

	err = smp_h6(w, key_id, res);
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

	err = smp_h7(salt, w, res);
	if (err) {
		return err;
	}

	if (memcmp(res, exp_res, 16)) {
		return -EINVAL;
	}

	return 0;
}
#endif /* CONFIG_BT_BREDR */

static int smp_self_test(void)
{
	int err;

	err = smp_aes_cmac_test();
	if (err) {
		BT_ERR("SMP AES-CMAC self tests failed");
		return err;
	}

	err = smp_sign_test();
	if (err) {
		BT_ERR("SMP signing self tests failed");
		return err;
	}

	err = smp_f4_test();
	if (err) {
		BT_ERR("SMP f4 self test failed");
		return err;
	}

	err = smp_f5_test();
	if (err) {
		BT_ERR("SMP f5 self test failed");
		return err;
	}

	err = smp_f6_test();
	if (err) {
		BT_ERR("SMP f6 self test failed");
		return err;
	}

	err = smp_g2_test();
	if (err) {
		BT_ERR("SMP g2 self test failed");
		return err;
	}

#if defined(CONFIG_BT_BREDR)
	err = smp_h6_test();
	if (err) {
		BT_ERR("SMP h6 self test failed");
		return err;
	}

	err = smp_h7_test();
	if (err) {
		BT_ERR("SMP h7 self test failed");
		return err;
	}
#endif /* CONFIG_BT_BREDR */

	return 0;
}
#else
static inline int smp_self_test(void)
{
	return 0;
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
	    smp->chan.chan.conn->role == BT_HCI_ROLE_MASTER) {
		err = smp_send_pairing_confirm(smp);
		if (err) {
			smp_error(smp, BT_SMP_ERR_PASSKEY_ENTRY_FAILED);
			return 0;
		}
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PAIRING_CONFIRM);
		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL) &&
	    atomic_test_bit(smp->flags, SMP_FLAG_CFM_DELAYED)) {
		err = smp_send_pairing_confirm(smp);
		if (err) {
			smp_error(smp, BT_SMP_ERR_PASSKEY_ENTRY_FAILED);
			return 0;
		}
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PAIRING_RANDOM);
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
		if (smp->chan.chan.conn->role == BT_HCI_ROLE_MASTER) {
			err = compute_and_send_master_dhcheck(smp);
			if (err) {
				smp_error(smp, err);
			}
			return 0;
		}
#endif /* CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_PERIPHERAL)
		err = compute_and_check_and_send_slave_dhcheck(smp);
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

	BT_DBG("%s", bt_hex(tk, 16));

	if (!atomic_test_and_clear_bit(smp->flags, SMP_FLAG_USER)) {
		return -EINVAL;
	}

	memcpy(smp->tk, tk, 16*sizeof(uint8_t));

	legacy_user_tk_entry(smp);

	return 0;
}
#endif /* !defined(CONFIG_BT_SMP_SC_PAIR_ONLY) */

#if !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)
int bt_smp_le_oob_generate_sc_data(struct bt_le_oob_sc_data *le_sc_oob)
{
	int err;

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

	err = smp_f4(sc_public_key, sc_public_key, le_sc_oob->r, 0,
		     le_sc_oob->c);
	if (err) {
		return err;
	}

	return 0;
}
#endif /* !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY) */

#if !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)
static bool le_sc_oob_data_check(struct bt_smp *smp, bool oobd_local_present,
				 bool oobd_remote_present)
{
	bool req_oob_present = le_sc_oob_data_req_check(smp);
	bool rsp_oob_present = le_sc_oob_data_rsp_check(smp);

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    smp->chan.chan.conn->role == BT_HCI_ROLE_MASTER) {
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

		err = smp_f4(smp->pkey, smp->pkey, smp->oobd_remote->r, 0, c);
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
	    smp->chan.chan.conn->role == BT_HCI_ROLE_MASTER) {
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PAIRING_RANDOM);
	} else if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_DHKEY_CHECK);
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
	    conn->role == BT_CONN_ROLE_MASTER) {
		if (!atomic_test_bit(smp->flags, SMP_FLAG_SC)) {
			atomic_set_bit(&smp->allowed_cmds,
				       BT_SMP_CMD_PAIRING_CONFIRM);
			return legacy_send_pairing_confirm(smp);
		}

		if (!sc_public_key) {
			atomic_set_bit(smp->flags, SMP_FLAG_PKEY_SEND);
			return 0;
		}

		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PUBLIC_KEY);
		return sc_send_public_key(smp);
	}

#if defined(CONFIG_BT_PERIPHERAL)
	if (!atomic_test_bit(smp->flags, SMP_FLAG_SC)) {
		atomic_set_bit(&smp->allowed_cmds,
			       BT_SMP_CMD_PAIRING_CONFIRM);
		return send_pairing_rsp(smp);
	}

	atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PUBLIC_KEY);
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
		passkey = BT_PASSKEY_INVALID;
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
	case BT_HCI_ROLE_MASTER:
	{
		int err;
		struct bt_smp *smp;

		smp = smp_chan_get(conn);
		if (!smp) {
			return -ENOTCONN;
		}

		if (!smp_keys_check(conn)) {
			return smp_send_pairing_req(conn);
		}

		/* pairing is in progress */
		if (atomic_test_bit(smp->flags, SMP_FLAG_PAIRING)) {
			return -EBUSY;
		}

		/* Encryption is in progress */
		if (atomic_test_bit(smp->flags, SMP_FLAG_ENC_PENDING)) {
			return -EBUSY;
		}

		/* LE SC LTK and legacy master LTK are stored in same place */
		err = bt_conn_le_start_encryption(conn,
						  conn->le.keys->ltk.rand,
						  conn->le.keys->ltk.ediv,
						  conn->le.keys->ltk.val,
						  conn->le.keys->enc_size);
		if (err) {
			return err;
		}

		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_SECURITY_REQUEST);
		atomic_set_bit(smp->flags, SMP_FLAG_ENC_PENDING);
		return 0;
	}
#endif /* CONFIG_BT_CENTRAL && CONFIG_BT_SMP */
#if defined(CONFIG_BT_PERIPHERAL)
	case BT_HCI_ROLE_SLAVE:
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
		BT_ERR("Unable to get keys for %s",
		       bt_addr_le_str(&conn->le.dst));
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
	case PASSKEY_DISPLAY:
	case PASSKEY_INPUT:
	case PASSKEY_CONFIRM:
	case LE_SC_OOB:
	case LEGACY_OOB:
		conn->le.keys->flags |= BT_KEYS_AUTHENTICATED;
		break;
	case JUST_WORKS:
	default:
		/* unauthenticated key, clear it */
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

	BT_DBG("conn %p handle %u", conn, conn->handle);

	for (i = 0; i < ARRAY_SIZE(bt_smp_pool); i++) {
		struct bt_smp *smp = &bt_smp_pool[i];

		if (smp->chan.chan.conn) {
			continue;
		}

		smp->chan.chan.ops = &ops;

		*chan = &smp->chan.chan;

		return 0;
	}

	BT_ERR("No available SMP context for conn %p", conn);

	return -ENOMEM;
}

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

BT_L2CAP_CHANNEL_DEFINE(smp_fixed_chan, BT_L2CAP_CID_SMP, bt_smp_accept, NULL);
#if defined(CONFIG_BT_BREDR)
BT_L2CAP_CHANNEL_DEFINE(smp_br_fixed_chan, BT_L2CAP_CID_BR_SMP,
			bt_smp_br_accept, NULL);
#endif /* CONFIG_BT_BREDR */

int bt_smp_init(void)
{
	static struct bt_pub_key_cb pub_key_cb = {
		.func           = bt_smp_pkey_ready,
	};

	sc_supported = le_sc_supported();
	if (IS_ENABLED(CONFIG_BT_SMP_SC_PAIR_ONLY) && !sc_supported) {
		BT_ERR("SC Pair Only Mode selected but LE SC not supported");
		return -ENOENT;
	}

	if (IS_ENABLED(CONFIG_BT_SMP_USB_HCI_CTLR_WORKAROUND)) {
		BT_WARN("BT_SMP_USB_HCI_CTLR_WORKAROUND is enabled, which "
			"exposes a security vulnerability!");
	}

	BT_DBG("LE SC %s", sc_supported ? "enabled" : "disabled");

	if (!IS_ENABLED(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)) {
		bt_pub_key_gen(&pub_key_cb);
	}

	return smp_self_test();
}
