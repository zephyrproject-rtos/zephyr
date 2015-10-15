/**
 * @file smp.c
 * Security Manager Protocol implementation
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <nanokernel.h>
#include <arch/cpu.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <atomic.h>
#include <misc/util.h>
#include <misc/byteorder.h>

#include <bluetooth/log.h>
#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>

#if defined(CONFIG_TINYCRYPT_AES)
#include <tinycrypt/aes.h>
#include <tinycrypt/utils.h>
#endif

#include "hci_core.h"
#include "keys.h"
#include "conn_internal.h"
#include "l2cap.h"
#include "smp.h"
#include "stack.h"

#define SMP_TIMEOUT (30 * sys_clock_ticks_per_sec)

#if defined(CONFIG_BLUETOOTH_SIGNING)
#define RECV_KEYS (BT_SMP_DIST_ID_KEY | BT_SMP_DIST_ENC_KEY | BT_SMP_DIST_SIGN)
#define SEND_KEYS (BT_SMP_DIST_ENC_KEY | BT_SMP_DIST_SIGN)
#else
#define RECV_KEYS (BT_SMP_DIST_ID_KEY | BT_SMP_DIST_ENC_KEY)
#define SEND_KEYS (BT_SMP_DIST_ENC_KEY)
#endif /* CONFIG_BLUETOOTH_SIGNING */

enum pairing_method {
	JUST_WORKS,		/* JustWorks pairing */
	PASSKEY_INPUT,		/* Passkey Entry input */
	PASSKEY_DISPLAY,	/* Passkey Entry display */
	PASSKEY_ROLE,		/* Passkey Entry depends on role */
} ;

enum {
	SMP_FLAG_TK_VALID,	/* if TK values is valid */
	SMP_FLAG_CFM_DELAYED,	/* if confirm should be send when TK is valid */
	SMP_FLAG_ENC_PENDING,	/* if waiting for an encryption change event */
	SMP_FLAG_PAIRING,	/* if pairing is in progress */
	SMP_FLAG_TIMEOUT,	/* if SMP timeout occurred */
};

/* SMP channel specific context */
struct bt_smp {
	/* The connection this context is associated with */
	struct bt_conn		*conn;

	/* SMP Timeout fiber handle */
	void			*timeout;

	/* Commands that remote is allowed to send */
	atomic_t		allowed_cmds;

	/* Flags for SMP state machine */
	atomic_t		flags;

	/* Type of method used for pairing */
	uint8_t			method;

	/* Pairing Request PDU */
	uint8_t			preq[7];

	/* Pairing Response PDU */
	uint8_t			prsp[7];

	/* Pairing Confirm PDU */
	uint8_t			pcnf[16];

	/* Local random number */
	uint8_t			prnd[16];

	/* Remote random number */
	uint8_t			rrnd[16];

	/* Temporary key */
	uint8_t			tk[16];

	/* Local key distribution */
	uint8_t			local_dist;

	/* Remote key distribution */
	uint8_t			remote_dist;

	/* stack for timeout fiber */
	BT_STACK(stack, 128);
};

/* based on table 2.8 Core Spec 2.3.5.1 Vol. 3 Part H */
static const uint8_t gen_method[5 /* remote */][5 /* local */] = {
	{ JUST_WORKS, JUST_WORKS, PASSKEY_INPUT, JUST_WORKS, PASSKEY_INPUT },
	{ JUST_WORKS, JUST_WORKS, PASSKEY_INPUT, JUST_WORKS, PASSKEY_INPUT },
	{ PASSKEY_DISPLAY, PASSKEY_DISPLAY, PASSKEY_INPUT, JUST_WORKS,
	  PASSKEY_DISPLAY },
	{ JUST_WORKS, JUST_WORKS, JUST_WORKS, JUST_WORKS, JUST_WORKS },
	{ PASSKEY_DISPLAY, PASSKEY_DISPLAY, PASSKEY_INPUT, JUST_WORKS,
	  PASSKEY_ROLE },
};

static struct bt_smp bt_smp_pool[CONFIG_BLUETOOTH_MAX_CONN];
static const struct bt_auth_cb *auth_cb;
static uint8_t bt_smp_io_capa = BT_SMP_IO_NO_INPUT_OUTPUT;

static uint8_t get_pair_method(struct bt_smp *smp, uint8_t remote_io)
{
	uint8_t method;

	if (remote_io > BT_SMP_IO_KEYBOARD_DISPLAY)
		return JUST_WORKS;

	method = gen_method[remote_io][bt_smp_io_capa];

	/* if both sides have KeyboardDisplay capabilities, initiator displays
	 * and responder inputs
	 */
	if (method == PASSKEY_ROLE) {
		if (smp->conn->role == BT_HCI_ROLE_MASTER) {
			method = PASSKEY_DISPLAY;
		} else {
			method = PASSKEY_INPUT;
		}
	}

	return method;
}

#if defined(CONFIG_BLUETOOTH_DEBUG_SMP)
/* Helper for printk parameters to convert from binary to hex.
 * We declare multiple buffers so the helper can be used multiple times
 * in a single printk call.
 */
static const char *h(const void *buf, size_t len)
{
	static const char hex[] = "0123456789abcdef";
	static char hexbufs[4][129];
	static uint8_t curbuf;
	const uint8_t *b = buf;
	char *str;
	int i;

	str = hexbufs[curbuf++];
	curbuf %= ARRAY_SIZE(hexbufs);

	len = min(len, (sizeof(hexbufs[0]) - 1) / 2);

	for (i = 0; i < len; i++) {
		str[i * 2]     = hex[b[i] >> 4];
		str[i * 2 + 1] = hex[b[i] & 0xf];
	}

	str[i * 2] = '\0';

	return str;
}
#else
#undef BT_DBG
#define BT_DBG(fmt, ...)
#endif

typedef struct {
	uint64_t a;
	uint64_t b;
} uint128_t;

static void xor_128(const uint128_t *p, const uint128_t *q, uint128_t *r)
{
	r->a = p->a ^ q->a;
	r->b = p->b ^ q->b;
}

#if defined(CONFIG_TINYCRYPT_AES) || defined(CONFIG_BLUETOOTH_SIGNING)
/* swap octets for LE encrypt */
static void swap_buf(const uint8_t *src, uint8_t *dst, uint16_t len)
{
	int i;

	for (i = 0; i < len; i++) {
		dst[len - 1 - i] = src[i];
	}
}

static void swap_in_place(uint8_t *buf, uint16_t len)
{
	int i, j;

	for (i = 0, j = len - 1; i < j; i++, j--) {
		uint8_t tmp = buf[i];

		buf[i] = buf[j];
		buf[j] = tmp;
	}
}
#endif /* CONFIG_TINYCRYPT_AES || CONFIG_BLUETOOTH_SIGNING */

#if defined(CONFIG_TINYCRYPT_AES)
static int le_encrypt(const uint8_t key[16], const uint8_t plaintext[16],
		      uint8_t enc_data[16])
{
	struct tc_aes_key_sched_struct s;
	uint8_t tmp[16];

	BT_DBG("key %s plaintext %s\n", h(key, 16), h(plaintext, 16));

	swap_buf(key, tmp, 16);

	if (tc_aes128_set_encrypt_key(&s, tmp) == TC_FAIL) {
		return -EINVAL;
	}

	swap_buf(plaintext, tmp, 16);

	if (tc_aes_encrypt(enc_data, tmp, &s) == TC_FAIL) {
		return -EINVAL;
	}

	swap_in_place(enc_data, 16);

	BT_DBG("enc_data %s\n", h(enc_data, 16));

	return 0;
}
#else
static int le_encrypt(const uint8_t key[16], const uint8_t plaintext[16],
		      uint8_t enc_data[16])
{
	struct bt_hci_cp_le_encrypt *cp;
	struct bt_hci_rp_le_encrypt *rp;
	struct bt_buf *buf, *rsp;
	int err;

	BT_DBG("key %s plaintext %s\n", h(key, 16), h(plaintext, 16));

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_ENCRYPT, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = bt_buf_add(buf, sizeof(*cp));
	memcpy(cp->key, key, sizeof(cp->key));
	memcpy(cp->plaintext, plaintext, sizeof(cp->plaintext));

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_ENCRYPT, buf, &rsp);
	if (err) {
		return err;
	}

	rp = (void *)rsp->data;
	memcpy(enc_data, rp->enc_data, sizeof(rp->enc_data));
	bt_buf_put(rsp);

	BT_DBG("enc_data %s\n", h(enc_data, 16));

	return 0;
}
#endif

static int le_rand(void *buf, size_t len)
{
	uint8_t *ptr = buf;

	while (len > 0) {
		struct bt_hci_rp_le_rand *rp;
		struct bt_buf *rsp;
		size_t copy;
		int err;

		err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_RAND, NULL, &rsp);
		if (err) {
			BT_ERR("HCI_LE_Random failed (%d)\n", err);
			return err;
		}

		rp = (void *)rsp->data;
		copy = min(len, sizeof(rp->rand));
		memcpy(ptr, rp->rand, copy);
		bt_buf_put(rsp);

		len -= copy;
		ptr += copy;
	}

	return 0;
}

static int smp_ah(const uint8_t irk[16], const uint8_t r[3], uint8_t out[3])
{
	uint8_t res[16];
	int err;

	BT_DBG("irk %s\n, r %s", h(irk, 16), h(r, 3));

	/* r' = padding || r */
	memcpy(res, r, 3);
	memset(res + 3, 0, 13);

	err = le_encrypt(irk, res, res);
	if (err) {
		return err;
	}

	/* The output of the random address function ah is:
	 *      ah(h, r) = e(k, r') mod 2^24
	 * The output of the security function e is then truncated to 24 bits
	 * by taking the least significant 24 bits of the output of e as the
	 * result of ah.
	 */
	memcpy(out, res, 3);

	return 0;
}

static int smp_c1(const uint8_t k[16], const uint8_t r[16],
		  const uint8_t preq[7], const uint8_t pres[7],
		  const bt_addr_le_t *ia, const bt_addr_le_t *ra,
		  uint8_t enc_data[16])
{
	uint8_t p1[16], p2[16];
	int err;

	BT_DBG("k %s r %s\n", h(k, 16), h(r, 16));
	BT_DBG("ia %s ra %s\n", bt_addr_le_str(ia), bt_addr_le_str(ra));
	BT_DBG("preq %s pres %s\n", h(preq, 7), h(pres, 7));

	/* pres, preq, rat and iat are concatenated to generate p1 */
	p1[0] = ia->type;
	p1[1] = ra->type;
	memcpy(p1 + 2, preq, 7);
	memcpy(p1 + 9, pres, 7);

	BT_DBG("p1 %s\n", h(p1, 16));

	/* c1 = e(k, e(k, r XOR p1) XOR p2) */

	/* Using enc_data as temporary output buffer */
	xor_128((uint128_t *)r, (uint128_t *)p1, (uint128_t *)enc_data);

	err = le_encrypt(k, enc_data, enc_data);
	if (err) {
		return err;
	}

	/* ra is concatenated with ia and padding to generate p2 */
	memcpy(p2, ra->val, 6);
	memcpy(p2 + 6, ia->val, 6);
	memset(p2 + 12, 0, 4);

	BT_DBG("p2 %s\n", h(p2, 16));

	xor_128((uint128_t *)enc_data, (uint128_t *)p2, (uint128_t *)enc_data);

	return le_encrypt(k, enc_data, enc_data);
}

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
	return le_encrypt(k, out, out);
}

static void smp_reset(struct bt_smp *smp)
{
	if (smp->timeout) {
		fiber_fiber_delayed_start_cancel(smp->timeout);
		smp->timeout = NULL;

		stack_analyze("smp timeout stack", smp->stack,
			      sizeof(smp->stack));
	}

	smp->method = JUST_WORKS;
	atomic_set(&smp->allowed_cmds, 0);
	atomic_set(&smp->flags, 0);

	if (smp->conn->required_sec_level != smp->conn->sec_level) {
		/* TODO report error */
		/* reset required security level in case of error */
		smp->conn->required_sec_level = smp->conn->sec_level;
	}

	switch(smp->conn->role) {
#if defined(CONFIG_BLUETOOTH_CENTRAL)
	case BT_HCI_ROLE_MASTER:
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_SECURITY_REQUEST);
		break;
#endif /* CONFIG_BLUETOOTH_CENTRAL */

#if defined(CONFIG_BLUETOOTH_PERIPHERAL)
	case BT_HCI_ROLE_SLAVE:
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PAIRING_REQ);
		break;
#endif /* CONFIG_BLUETOOTH_PERIPHERAL */
	default:
		break;
	}
}

static void smp_timeout(int arg1, int arg2)
{
	struct bt_smp *smp = (struct bt_smp *)arg1;
	ARG_UNUSED(arg2);

	BT_ERR("SMP Timeout\n");

	smp->timeout = NULL;

	smp_reset(smp);

	atomic_set_bit(&smp->flags, SMP_FLAG_TIMEOUT);
}

static void smp_restart_timer(struct bt_smp *smp)
{
	if (smp->timeout) {
		fiber_fiber_delayed_start_cancel(smp->timeout);
	}

	smp->timeout = fiber_delayed_start(smp->stack, sizeof(smp->stack),
					   smp_timeout, (int) smp, 0, 7, 0,
					   SMP_TIMEOUT);
}

struct bt_buf *bt_smp_create_pdu(struct bt_conn *conn, uint8_t op, size_t len)
{
	struct bt_smp_hdr *hdr;
	struct bt_buf *buf;

	buf = bt_l2cap_create_pdu(conn);
	if (!buf) {
		return NULL;
	}

	hdr = bt_buf_add(buf, sizeof(*hdr));
	hdr->code = op;

	return buf;
}

static void send_err_rsp(struct bt_conn *conn, uint8_t reason)
{
	struct bt_smp_pairing_fail *rsp;
	struct bt_buf *buf;

	buf = bt_smp_create_pdu(conn, BT_SMP_CMD_PAIRING_FAIL, sizeof(*rsp));
	if (!buf) {
		return;
	}

	rsp = bt_buf_add(buf, sizeof(*rsp));
	rsp->reason = reason;

	bt_l2cap_send(conn, BT_L2CAP_CID_SMP, buf);
}

static int smp_init(struct bt_smp *smp)
{
	struct bt_conn *conn = smp->conn;

	/* Initialize SMP context */
	memset(smp, 0, sizeof(*smp));

	/* Generate local random number */
	if (le_rand(smp->prnd, 16)) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	BT_DBG("prnd %s\n", h(smp->prnd, 16));

	smp->conn = conn;

	atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PAIRING_FAIL);

	return 0;
}

static uint8_t get_auth(uint8_t auth)
{
	auth &= BT_SMP_AUTH_MASK;

	if (bt_smp_io_capa == BT_SMP_IO_NO_INPUT_OUTPUT) {
		auth &= ~(BT_SMP_AUTH_MITM);
	} else {
		auth |= BT_SMP_AUTH_MITM;
	}

	return auth;
}

static uint8_t smp_request_tk(struct bt_smp *smp, uint8_t remote_io)
{
	struct bt_keys *keys;
	uint32_t passkey;

	smp->method = get_pair_method(smp, remote_io);

	/* Fail if we have keys that are stronger than keys that will be
	 * distributed in new pairing. This is to avoid replacing authenticated
	 * keys with unauthenticated ones.
	  */
	keys = bt_keys_find_addr(&smp->conn->dst);
	if (keys && keys->type == BT_KEYS_AUTHENTICATED &&
	    smp->method == JUST_WORKS) {
		BT_ERR("JustWorks failed, authenticated keys present\n");
		return BT_SMP_ERR_UNSPECIFIED;
	}

	switch (smp->method) {
	case PASSKEY_DISPLAY:
		if (le_rand(&passkey, sizeof(passkey))) {
			return BT_SMP_ERR_UNSPECIFIED;
		}

		passkey %= 1000000;

		auth_cb->passkey_display(smp->conn, passkey);

		passkey = sys_cpu_to_le32(passkey);
		memcpy(smp->tk, &passkey, sizeof(passkey));
		atomic_set_bit(&smp->flags, SMP_FLAG_TK_VALID);

		break;
	case PASSKEY_INPUT:
		auth_cb->passkey_entry(smp->conn);
		break;
	case JUST_WORKS:
		atomic_set_bit(&smp->flags, SMP_FLAG_TK_VALID);
		break;
	default:
		BT_ERR("Unknown pairing method (%u)\n", smp->method);
		return BT_SMP_ERR_UNSPECIFIED;
	}

	return 0;
}

static bool sec_level_reachable(struct bt_conn *conn)
{
	switch (conn->required_sec_level) {
	case BT_SECURITY_LOW:
	case BT_SECURITY_MEDIUM:
		return true;
	case BT_SECURITY_HIGH:
		return bt_smp_io_capa != BT_SMP_IO_NO_INPUT_OUTPUT;
	case BT_SECURITY_FIPS:
	default:
		return false;
	}
}

#if defined(CONFIG_BLUETOOTH_PERIPHERAL)
int bt_smp_send_security_req(struct bt_conn *conn)
{
	struct bt_smp *smp = conn->smp;
	struct bt_smp_security_request *req;
	struct bt_buf *req_buf;

	BT_DBG("\n");

	/* SMP Timeout */
	if (atomic_test_bit(&smp->flags, SMP_FLAG_TIMEOUT)) {
		return -EIO;
	}

	/* pairing is in progress */
	if (atomic_test_bit(&smp->flags, SMP_FLAG_PAIRING)) {
		return -EBUSY;
	}

	/* early verify if required sec level if reachable */
	if (!sec_level_reachable(conn)) {
		return -EINVAL;
	}

	req_buf = bt_smp_create_pdu(conn, BT_SMP_CMD_SECURITY_REQUEST,
				    sizeof(*req));
	if (!req_buf) {
		return -ENOBUFS;
	}

	req = bt_buf_add(req_buf, sizeof(*req));

	req->auth_req = BT_SMP_AUTH_BONDING;

	if (conn->required_sec_level >= BT_SECURITY_HIGH) {
		req->auth_req |= BT_SMP_AUTH_MITM;
	}

	bt_l2cap_send(conn, BT_L2CAP_CID_SMP, req_buf);

	atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PAIRING_FAIL);

	return 0;
}

static uint8_t smp_pairing_req(struct bt_conn *conn, struct bt_buf *buf)
{
	struct bt_smp_pairing *req = (void *)buf->data;
	struct bt_smp_pairing *rsp;
	struct bt_buf *rsp_buf;
	struct bt_smp *smp = conn->smp;
	int ret;

	BT_DBG("\n");

	if ((req->max_key_size > BT_SMP_MAX_ENC_KEY_SIZE) ||
	    (req->max_key_size < BT_SMP_MIN_ENC_KEY_SIZE)) {
		return BT_SMP_ERR_ENC_KEY_SIZE;
	}

	ret = smp_init(smp);
	if (ret) {
		return ret;
	}

	rsp_buf = bt_smp_create_pdu(conn, BT_SMP_CMD_PAIRING_RSP, sizeof(*rsp));
	if (!rsp_buf) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	rsp = bt_buf_add(rsp_buf, sizeof(*rsp));

	rsp->auth_req = get_auth(req->auth_req);
	rsp->io_capability = bt_smp_io_capa;
	rsp->oob_flag = BT_SMP_OOB_NOT_PRESENT;
	rsp->max_key_size = BT_SMP_MAX_ENC_KEY_SIZE;
	rsp->init_key_dist = (req->init_key_dist & RECV_KEYS);
	rsp->resp_key_dist = (req->resp_key_dist & SEND_KEYS);

	smp->local_dist = rsp->resp_key_dist;
	smp->remote_dist = rsp->init_key_dist;

	/* Store req/rsp for later use */
	smp->preq[0] = BT_SMP_CMD_PAIRING_REQ;
	memcpy(smp->preq + 1, req, sizeof(*req));
	smp->prsp[0] = BT_SMP_CMD_PAIRING_RSP;
	memcpy(smp->prsp + 1, rsp, sizeof(*rsp));

	bt_l2cap_send(conn, BT_L2CAP_CID_SMP, rsp_buf);

	atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PAIRING_CONFIRM);
	atomic_set_bit(&smp->flags, SMP_FLAG_PAIRING);

	smp_restart_timer(smp);

	return smp_request_tk(smp, req->io_capability);
}
#else
static uint8_t smp_pairing_req(struct bt_conn *conn, struct bt_buf *buf)
{
	return BT_SMP_ERR_CMD_NOTSUPP;
}
#endif /* CONFIG_BLUETOOTH_PERIPHERAL */

static uint8_t smp_send_pairing_confirm(struct bt_conn *conn)
{
	struct bt_smp_pairing_confirm *req;
	struct bt_smp *smp = conn->smp;
	struct bt_buf *rsp_buf;
	int err;

	rsp_buf = bt_smp_create_pdu(conn, BT_SMP_CMD_PAIRING_CONFIRM,
				    sizeof(*req));
	if (!rsp_buf) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	req = bt_buf_add(rsp_buf, sizeof(*req));

	err = smp_c1(smp->tk, smp->prnd, smp->preq, smp->prsp, &conn->init_addr,
		     &conn->resp_addr, req->val);
	if (err) {
		bt_buf_put(rsp_buf);
		return BT_SMP_ERR_UNSPECIFIED;
	}

	bt_l2cap_send(conn, BT_L2CAP_CID_SMP, rsp_buf);

	atomic_clear_bit(&smp->flags, SMP_FLAG_CFM_DELAYED);

	smp_restart_timer(smp);

	return 0;
}

#if defined(CONFIG_BLUETOOTH_CENTRAL)
int bt_smp_send_pairing_req(struct bt_conn *conn)
{
	struct bt_smp *smp = conn->smp;
	struct bt_smp_pairing *req;
	struct bt_buf *req_buf;

	BT_DBG("\n");

	/* SMP Timeout */
	if (atomic_test_bit(&smp->flags, SMP_FLAG_TIMEOUT)) {
		return -EIO;
	}

	/* pairing is in progress */
	if (atomic_test_bit(&smp->flags, SMP_FLAG_PAIRING)) {
		return -EBUSY;
	}

	/* early verify if required sec level if reachable */
	if (!sec_level_reachable(conn)) {
		return -EINVAL;
	}

	if (smp_init(smp)) {
		return -ENOBUFS;
	}

	req_buf = bt_smp_create_pdu(conn, BT_SMP_CMD_PAIRING_REQ, sizeof(*req));
	if (!req_buf) {
		return -ENOBUFS;
	}

	req = bt_buf_add(req_buf, sizeof(*req));

	req->auth_req = get_auth(BT_SMP_AUTH_BONDING);
	req->io_capability = bt_smp_io_capa;
	req->oob_flag = BT_SMP_OOB_NOT_PRESENT;
	req->max_key_size = BT_SMP_MAX_ENC_KEY_SIZE;
	req->init_key_dist = SEND_KEYS;
	req->resp_key_dist = RECV_KEYS;

	smp->local_dist = SEND_KEYS;
	smp->remote_dist = RECV_KEYS;

	/* Store req for later use */
	smp->preq[0] = BT_SMP_CMD_PAIRING_REQ;
	memcpy(smp->preq + 1, req, sizeof(*req));

	bt_l2cap_send(conn, BT_L2CAP_CID_SMP, req_buf);

	atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PAIRING_RSP);
	atomic_set_bit(&smp->flags, SMP_FLAG_PAIRING);

	smp_restart_timer(smp);

	return 0;
}

static uint8_t smp_pairing_rsp(struct bt_conn *conn, struct bt_buf *buf)
{
	struct bt_smp_pairing *rsp = (void *)buf->data;
	struct bt_smp *smp = conn->smp;
	uint8_t ret;

	BT_DBG("\n");

	if ((rsp->max_key_size > BT_SMP_MAX_ENC_KEY_SIZE) ||
	    (rsp->max_key_size < BT_SMP_MIN_ENC_KEY_SIZE)) {
		return BT_SMP_ERR_ENC_KEY_SIZE;
	}

	smp->local_dist &= rsp->init_key_dist;
	smp->remote_dist &= rsp->resp_key_dist;

	/* Store rsp for later use */
	smp->prsp[0] = BT_SMP_CMD_PAIRING_RSP;
	memcpy(smp->prsp + 1, rsp, sizeof(*rsp));

	ret = smp_request_tk(smp, rsp->io_capability);
	if (ret) {
		return ret;
	}

	if (atomic_test_bit(&smp->flags, SMP_FLAG_TK_VALID)) {
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PAIRING_CONFIRM);
		return smp_send_pairing_confirm(conn);
	}

	atomic_set_bit(&smp->flags, SMP_FLAG_CFM_DELAYED);

	return 0;
}
#else
static uint8_t smp_pairing_rsp(struct bt_conn *conn, struct bt_buf *buf)
{
	return BT_SMP_ERR_CMD_NOTSUPP;
}
#endif /* CONFIG_BLUETOOTH_CENTRAL */

static uint8_t smp_send_pairing_random(struct bt_conn *conn)
{
	struct bt_smp_pairing_random *req;
	struct bt_buf *rsp_buf;
	struct bt_smp *smp = conn->smp;

	rsp_buf = bt_smp_create_pdu(conn, BT_SMP_CMD_PAIRING_RANDOM,
				    sizeof(*req));
	if (!rsp_buf) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	req = bt_buf_add(rsp_buf, sizeof(*req));
	memcpy(req->val, smp->prnd, sizeof(req->val));

	bt_l2cap_send(conn, BT_L2CAP_CID_SMP, rsp_buf);

	smp_restart_timer(smp);

	return 0;
}

static uint8_t smp_pairing_confirm(struct bt_conn *conn, struct bt_buf *buf)
{
	struct bt_smp_pairing_confirm *req = (void *)buf->data;
	struct bt_smp *smp = conn->smp;

	BT_DBG("\n");

	memcpy(smp->pcnf, req->val, sizeof(smp->pcnf));

#if defined(CONFIG_BLUETOOTH_CENTRAL)
	if (conn->role == BT_HCI_ROLE_MASTER) {
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PAIRING_RANDOM);
		return smp_send_pairing_random(conn);
	}
#endif /* CONFIG_BLUETOOTH_CENTRAL */

#if defined(CONFIG_BLUETOOTH_PERIPHERAL)
	if (atomic_test_bit(&smp->flags, SMP_FLAG_TK_VALID)) {
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PAIRING_RANDOM);
		return smp_send_pairing_confirm(conn);
	}

	atomic_set_bit(&smp->flags, SMP_FLAG_CFM_DELAYED);
#endif /* CONFIG_BLUETOOTH_PERIPHERAL */
	return 0;
}

static uint8_t get_keys_type(uint8_t method)
{
	switch (method) {
	case PASSKEY_DISPLAY:
	case PASSKEY_INPUT:
		return BT_KEYS_AUTHENTICATED;
	case JUST_WORKS:
	default:
		return BT_KEYS_UNAUTHENTICATED;
	}
}

static uint8_t get_encryption_key_size(struct bt_smp *smp)
{
	struct bt_smp_pairing *req, *rsp;

	req = (struct bt_smp_pairing *)&smp->preq[1];
	rsp = (struct bt_smp_pairing *)&smp->prsp[1];

	/* The smaller value of the initiating and responding devices maximum
	 * encryption key length parameters shall be used as the encryption key
	 * size.
	 */
	return min(req->max_key_size, rsp->max_key_size);
}

static uint8_t smp_pairing_random(struct bt_conn *conn, struct bt_buf *buf)
{
	struct bt_smp_pairing_random *req = (void *)buf->data;
	struct bt_smp *smp = conn->smp;
	struct bt_keys *keys;
	uint8_t cfm[16];
	int err;

	BT_DBG("\n");

	memcpy(smp->rrnd, req->val, sizeof(smp->rrnd));

	err = smp_c1(smp->tk, smp->rrnd, smp->preq, smp->prsp, &conn->init_addr,
		     &conn->resp_addr, cfm);
	if (err) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	BT_DBG("pcnf %s cfm %s\n", h(smp->pcnf, 16), h(cfm, 16));

	if (memcmp(smp->pcnf, cfm, sizeof(smp->pcnf))) {
		return BT_SMP_ERR_CONFIRM_FAILED;
	}

#if defined(CONFIG_BLUETOOTH_CENTRAL)
	if (conn->role == BT_HCI_ROLE_MASTER) {
		uint8_t stk[16];

		/* No need to store master STK */
		err = smp_s1(smp->tk, smp->rrnd, smp->prnd, stk);
		if (err) {
			return BT_SMP_ERR_UNSPECIFIED;
		}

		keys = bt_keys_get_addr(&conn->dst);
		if (!keys) {
			return BT_SMP_ERR_UNSPECIFIED;
		}

		/* store key type deducted from pairing method used
		 * it is important to store it since type is used to determine
		 * security level upon encryption
		 */
		keys->type = get_keys_type(smp->method);
		keys->enc_size = get_encryption_key_size(smp);

		/* Rand and EDiv are 0 for the STK */
		if (bt_conn_le_start_encryption(conn, 0, 0, stk,
						keys->enc_size)) {
			BT_ERR("Failed to start encryption\n");
			return BT_SMP_ERR_UNSPECIFIED;
		}

		atomic_set_bit(&smp->flags, SMP_FLAG_ENC_PENDING);

		return 0;
	}
#endif /* CONFIG_BLUETOOTH_CENTRAL */

#if defined(CONFIG_BLUETOOTH_PERIPHERAL)
	keys = bt_keys_get_type(BT_KEYS_SLAVE_LTK, &conn->dst);
	if (!keys) {
		BT_ERR("Unable to create new keys\n");
		return BT_SMP_ERR_UNSPECIFIED;
	}

	err = smp_s1(smp->tk, smp->prnd, smp->rrnd, keys->slave_ltk.val);
	if (err) {
		bt_keys_clear(keys, BT_KEYS_SLAVE_LTK);
		return BT_SMP_ERR_UNSPECIFIED;
	}

	/* store key type deducted from pairing method used
	 * it is important to store it since type is used to determine
	 * security level upon encryption
	 */
	keys->type = get_keys_type(smp->method);
	keys->enc_size = get_encryption_key_size(smp);

	/* Rand and EDiv are 0 for the STK */
	keys->slave_ltk.rand = 0;
	keys->slave_ltk.ediv = 0;

	BT_DBG("generated STK %s\n", h(keys->slave_ltk.val, 16));

	atomic_set_bit(&smp->flags, SMP_FLAG_ENC_PENDING);

	smp_send_pairing_random(conn);
#endif /* CONFIG_BLUETOOTH_PERIPHERAL */

	return 0;
}

static uint8_t smp_pairing_failed(struct bt_conn *conn, struct bt_buf *buf)
{
	struct bt_smp_pairing_fail *req = (void *)buf->data;
	struct bt_smp *smp = conn->smp;

	BT_ERR("reason 0x%x\n", req->reason);

	/* TODO report error
	 * for now this to avoid warning about unused variable when debugs are
	 * disabled
	 */
	ARG_UNUSED(req);

	switch (smp->method) {
	case PASSKEY_INPUT:
	case PASSKEY_DISPLAY:
		auth_cb->cancel(conn);
		break;
	default:
		break;
	}

	smp_reset(smp);

	/* return no error to avoid sending Pairing Failed in response */
	return 0;
}

static void bt_smp_distribute_keys(struct bt_conn *conn)
{
	struct bt_smp *smp = conn->smp;
	struct bt_keys *keys;
	struct bt_buf *buf;

	keys = bt_keys_get_addr(&conn->dst);
	if (!keys) {
		BT_ERR("Unable to look up keys for %s\n",
		       bt_addr_le_str(&conn->dst));
		return;
	}

	/* Store key type deducted from pairing method used. It is important
	 * to restore it here since all key info is cleared after encryption
	 * was enabled before key distribution.
	 *
	 * All keys from pairing have same type so it is enough to store it
	 * on local distribution only.
	 */
	keys->type = get_keys_type(smp->method);
	keys->enc_size = get_encryption_key_size(smp);

	if (smp->local_dist & BT_SMP_DIST_ENC_KEY) {
		struct bt_smp_encrypt_info *info;
		struct bt_smp_master_ident *ident;

		smp->local_dist &= ~BT_SMP_DIST_ENC_KEY;

		bt_keys_add_type(keys, BT_KEYS_SLAVE_LTK);

		le_rand(keys->slave_ltk.val, sizeof(keys->slave_ltk.val));
		le_rand(&keys->slave_ltk.rand, sizeof(keys->slave_ltk.rand));
		le_rand(&keys->slave_ltk.ediv, sizeof(keys->slave_ltk.ediv));

		buf = bt_smp_create_pdu(conn, BT_SMP_CMD_ENCRYPT_INFO,
					sizeof(*info));
		if (!buf) {
			BT_ERR("Unable to allocate Encrypt Info buffer\n");
			return;
		}

		info = bt_buf_add(buf, sizeof(*info));

		/* distributed only enc_size bytes of key */
		memcpy(info->ltk, keys->slave_ltk.val, keys->enc_size);
		if (keys->enc_size < sizeof(info->ltk)) {
			memset(info->ltk + keys->enc_size, 0,
			       sizeof(info->ltk) - keys->enc_size);
		}

		bt_l2cap_send(conn, BT_L2CAP_CID_SMP, buf);

		buf = bt_smp_create_pdu(conn, BT_SMP_CMD_MASTER_IDENT,
					sizeof(*ident));
		if (!buf) {
			BT_ERR("Unable to allocate Master Ident buffer\n");
			return;
		}

		ident = bt_buf_add(buf, sizeof(*ident));
		ident->rand = keys->slave_ltk.rand;
		ident->ediv = keys->slave_ltk.ediv;

		bt_l2cap_send(conn, BT_L2CAP_CID_SMP, buf);

		smp_restart_timer(smp);
	}

#if defined(CONFIG_BLUETOOTH_SIGNING)
	if (smp->local_dist & BT_SMP_DIST_SIGN) {
		struct bt_smp_signing_info *info;

		smp->local_dist &= ~BT_SMP_DIST_SIGN;

		bt_keys_add_type(keys, BT_KEYS_LOCAL_CSRK);

		le_rand(keys->local_csrk.val, sizeof(keys->local_csrk.val));
		keys->local_csrk.cnt = 0;

		buf = bt_smp_create_pdu(conn, BT_SMP_CMD_SIGNING_INFO,
					sizeof(*info));
		if (!buf) {
			BT_ERR("Unable to allocate Signing Info buffer\n");
			return;
		}

		info = bt_buf_add(buf, sizeof(*info));
		memcpy(info->csrk, keys->local_csrk.val, sizeof(info->csrk));

		bt_l2cap_send(conn, BT_L2CAP_CID_SMP, buf);

		smp_restart_timer(smp);
	}
#endif /* CONFIG_BLUETOOTH_SIGNING */
}

static uint8_t smp_encrypt_info(struct bt_conn *conn, struct bt_buf *buf)
{
	struct bt_smp_encrypt_info *req = (void *)buf->data;
	struct bt_smp *smp = conn->smp;
	struct bt_keys *keys;

	BT_DBG("\n");

	keys = bt_keys_get_type(BT_KEYS_LTK, &conn->dst);
	if (!keys) {
		BT_ERR("Unable to get keys for %s\n",
		       bt_addr_le_str(&conn->dst));
		return BT_SMP_ERR_UNSPECIFIED;
	}

	memcpy(keys->ltk.val, req->ltk, 16);

	atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_MASTER_IDENT);

	return 0;
}

static uint8_t smp_master_ident(struct bt_conn *conn, struct bt_buf *buf)
{
	struct bt_smp_master_ident *req = (void *)buf->data;
	struct bt_smp *smp = conn->smp;
	struct bt_keys *keys;

	BT_DBG("\n");

	keys = bt_keys_get_type(BT_KEYS_LTK, &conn->dst);
	if (!keys) {
		BT_ERR("Unable to get keys for %s\n",
		       bt_addr_le_str(&conn->dst));
		return BT_SMP_ERR_UNSPECIFIED;
	}

	keys->ltk.ediv = req->ediv;
	keys->ltk.rand = req->rand;

	smp->remote_dist &= ~BT_SMP_DIST_ENC_KEY;

	if (smp->remote_dist & BT_SMP_DIST_ID_KEY) {
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_IDENT_INFO);
	} else if (smp->remote_dist & BT_SMP_DIST_SIGN) {
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_SIGNING_INFO);
	}

#if defined(CONFIG_BLUETOOTH_CENTRAL)
	if (conn->role == BT_HCI_ROLE_MASTER && !smp->remote_dist) {
		bt_smp_distribute_keys(conn);
	}
#endif /* CONFIG_BLUETOOTH_CENTRAL */

	/* if all keys were distributed, pairing is done */
	if (!smp->local_dist && !smp->remote_dist) {
		smp_reset(smp);
	}

	return 0;
}

static uint8_t smp_ident_info(struct bt_conn *conn, struct bt_buf *buf)
{
	struct bt_smp_ident_info *req = (void *)buf->data;
	struct bt_smp *smp = conn->smp;
	struct bt_keys *keys;

	BT_DBG("\n");

	keys = bt_keys_get_type(BT_KEYS_IRK, &conn->dst);
	if (!keys) {
		BT_ERR("Unable to get keys for %s\n",
		       bt_addr_le_str(&conn->dst));
		return BT_SMP_ERR_UNSPECIFIED;
	}

	memcpy(keys->irk.val, req->irk, 16);

	atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_IDENT_ADDR_INFO);

	return 0;
}

static uint8_t smp_ident_addr_info(struct bt_conn *conn, struct bt_buf *buf)
{
	struct bt_smp_ident_addr_info *req = (void *)buf->data;
	struct bt_smp *smp = conn->smp;
	const bt_addr_le_t *dst;
	struct bt_keys *keys;

	BT_DBG("identity %s\n", bt_addr_le_str(&req->addr));

	if (!bt_addr_le_is_identity(&req->addr)) {
		BT_ERR("Invalid identity %s for %s\n",
		       bt_addr_le_str(&req->addr), bt_addr_le_str(&conn->dst));
		return BT_SMP_ERR_INVALID_PARAMS;
	}

	keys = bt_keys_get_type(BT_KEYS_IRK, &conn->dst);
	if (!keys) {
		BT_ERR("Unable to get keys for %s\n",
		       bt_addr_le_str(&conn->dst));
		return BT_SMP_ERR_UNSPECIFIED;
	}

	/* We can't use conn->dst here as this might already contain identity
	 * address known from previous pairing. Since all keys are cleared on
	 * re-pairing we wouldn't store IRK distributed in new pairing.
	 */
	if (conn->role == BT_HCI_ROLE_MASTER) {
		dst = &conn->resp_addr;
	} else {
		dst = &conn->init_addr;
	}

	if (bt_addr_le_is_rpa(dst)) {
		/* always update last use RPA */
		bt_addr_copy(&keys->irk.rpa, (bt_addr_t *)&dst->val);

		/* Update connection address and notify about identity
		 * resolved only if connection wasn't already reported with
		 * identity address. This may happen if IRK was present before
		 * ie. due to re-pairing.
		 */
		if (!bt_addr_le_is_identity(&conn->dst)) {
			bt_addr_le_copy(&keys->addr, &req->addr);
			bt_addr_le_copy(&conn->dst, &req->addr);

			bt_conn_identity_resolved(conn);
		}
	}

	smp->remote_dist &= ~BT_SMP_DIST_ID_KEY;

	if (smp->remote_dist & BT_SMP_DIST_SIGN) {
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_SIGNING_INFO);
	}

#if defined(CONFIG_BLUETOOTH_CENTRAL)
	if (conn->role == BT_HCI_ROLE_MASTER && !smp->remote_dist) {
		bt_smp_distribute_keys(conn);
	}
#endif /* CONFIG_BLUETOOTH_CENTRAL */

	/* if all keys were distributed, pairing is done */
	if (!smp->local_dist && !smp->remote_dist) {
		smp_reset(smp);
	}

	return 0;
}

#if defined(CONFIG_BLUETOOTH_SIGNING)
static uint8_t smp_signing_info(struct bt_conn *conn, struct bt_buf *buf)
{
	struct bt_smp_signing_info *req = (void *)buf->data;
	struct bt_smp *smp = conn->smp;
	struct bt_keys *keys;

	BT_DBG("\n");

	keys = bt_keys_get_type(BT_KEYS_REMOTE_CSRK, &conn->dst);
	if (!keys) {
		BT_ERR("Unable to get keys for %s\n",
		       bt_addr_le_str(&conn->dst));
		return BT_SMP_ERR_UNSPECIFIED;
	}

	memcpy(keys->remote_csrk.val, req->csrk, sizeof(keys->remote_csrk.val));

	smp->remote_dist &= ~BT_SMP_DIST_SIGN;

#if defined(CONFIG_BLUETOOTH_CENTRAL)
	if (conn->role == BT_HCI_ROLE_MASTER && !smp->remote_dist) {
		bt_smp_distribute_keys(conn);
	}
#endif /* CONFIG_BLUETOOTH_CENTRAL */

	/* if all keys were distributed, pairing is done */
	if (!smp->local_dist && !smp->remote_dist) {
		smp_reset(smp);
	}

	return 0;
}
#else
static uint8_t smp_signing_info(struct bt_conn *conn, struct bt_buf *buf)
{
	return BT_SMP_ERR_CMD_NOTSUPP;
}
#endif /* CONFIG_BLUETOOTH_SIGNING */

#if defined(CONFIG_BLUETOOTH_CENTRAL)
static uint8_t smp_security_request(struct bt_conn *conn, struct bt_buf *buf)
{
	struct bt_smp_security_request *req = (void *)buf->data;
	struct bt_smp *smp = conn->smp;
	struct bt_keys *keys;
	uint8_t auth;

	BT_DBG("\n");

	keys = bt_keys_find(BT_KEYS_LTK, &conn->dst);
	if (!keys) {
		goto pair;
	}

	auth = req->auth_req & BT_SMP_AUTH_MASK;

	if ((auth & BT_SMP_AUTH_MITM) && keys->type != BT_KEYS_AUTHENTICATED) {
		if (bt_smp_io_capa != BT_SMP_IO_NO_INPUT_OUTPUT) {
			BT_INFO("New auth requirements: 0x%x, repairing\n",
				auth);
			goto pair;
		}

		BT_WARN("Unsupported auth requirements: 0x%x, repairing\n",
			auth);
		goto pair;
	}

	if (bt_conn_le_start_encryption(conn, keys->ltk.rand, keys->ltk.ediv,
					keys->ltk.val, keys->enc_size) < 0) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	atomic_set_bit(&smp->flags, SMP_FLAG_ENC_PENDING);

	return 0;
pair:
	if (bt_smp_send_pairing_req(conn) < 0) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	return 0;
}
#else
static uint8_t smp_security_request(struct bt_conn *conn, struct bt_buf *buf)
{
	return BT_SMP_ERR_CMD_NOTSUPP;
}
#endif /* CONFIG_BLUETOOTH_CENTRAL */

static const struct {
	uint8_t  (*func)(struct bt_conn *conn, struct bt_buf *buf);
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
};

static void bt_smp_recv(struct bt_conn *conn, struct bt_buf *buf)
{
	struct bt_smp_hdr *hdr = (void *)buf->data;
	struct bt_smp *smp = conn->smp;
	uint8_t err;

	if (buf->len < sizeof(*hdr)) {
		BT_ERR("Too small SMP PDU received\n");
		goto done;
	}

	BT_DBG("Received SMP code 0x%02x len %u\n", hdr->code, buf->len);

	bt_buf_pull(buf, sizeof(*hdr));

	/*
	 * If SMP timeout occurred "no further SMP commands shall be sent over
	 * the L2CAP Security Manager Channel. A new SM procedure shall only be
	 * performed when a new physical link has been established."
	 */
	if (atomic_test_bit(&smp->flags, SMP_FLAG_TIMEOUT)) {
		BT_WARN("SMP command (code 0x%02x) received after timeout\n",
			hdr->code);
		goto done;
	}

	if (hdr->code >= ARRAY_SIZE(handlers) || !handlers[hdr->code].func) {
		BT_WARN("Unhandled SMP code 0x%02x\n", hdr->code);
		err = BT_SMP_ERR_CMD_NOTSUPP;
	} else {
		if (!atomic_test_and_clear_bit(&smp->allowed_cmds, hdr->code)) {
			BT_WARN("Unexpected SMP code 0x%02x\n", hdr->code);
			goto done;
		}

		if (buf->len != handlers[hdr->code].expect_len) {
			BT_ERR("Invalid len %u for code 0x%02x\n", buf->len,
			       hdr->code);
			err = BT_SMP_ERR_INVALID_PARAMS;
		} else {
			err = handlers[hdr->code].func(conn, buf);
		}
	}

	if (err) {
		send_err_rsp(conn, err);

		smp_reset(smp);
	}

done:
	bt_buf_put(buf);
}

static void bt_smp_connected(struct bt_conn *conn)
{
	int i;

	BT_DBG("conn %p handle %u\n", conn, conn->handle);

	for (i = 0; i < ARRAY_SIZE(bt_smp_pool); i++) {
		struct bt_smp *smp = &bt_smp_pool[i];

		if (smp->conn) {
			continue;
		}

		smp->conn = conn;
		conn->smp = smp;

		smp_reset(smp);

		return;
	}

	BT_ERR("No available SMP context for conn %p\n", conn);
}

static void bt_smp_disconnected(struct bt_conn *conn)
{
	struct bt_smp *smp = conn->smp;

	if (!smp) {
		return;
	}

	BT_DBG("conn %p handle %u\n", conn, conn->handle);

	conn->smp = NULL;

	if (smp->timeout) {
		fiber_fiber_delayed_start_cancel(smp->timeout);
	}

	memset(smp, 0, sizeof(*smp));
}

static void bt_smp_encrypt_change(struct bt_conn *conn)
{
	struct bt_smp *smp = conn->smp;
	struct bt_keys *keys;

	BT_DBG("conn %p handle %u encrypt 0x%02x\n", conn, conn->handle,
	        conn->encrypt);

	if (!smp || !conn->encrypt) {
		return;
	}

	if (!atomic_test_and_clear_bit(&smp->flags, SMP_FLAG_ENC_PENDING)) {
		return;
	}

	/* We were waiting for encryption but with no pairing in progress.
	 * This can happen if paired slave sent Security Request and we
	 * enabled encryption.
	 *
	 * Since it is possible that slave might sent another Security Request
	 * eg with different AuthReq we should allow it.
	 */
	if (!atomic_test_bit(&smp->flags, SMP_FLAG_PAIRING)) {
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_SECURITY_REQUEST);
		return;
	}

	if (smp->remote_dist & BT_SMP_DIST_ENC_KEY) {
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_ENCRYPT_INFO);
	} else if (smp->remote_dist & BT_SMP_DIST_ID_KEY) {
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_IDENT_INFO);
	} else if (smp->remote_dist & BT_SMP_DIST_SIGN) {
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_SIGNING_INFO);
	}

	/* If link was successfully encrypted cleanup old keys as from now on
	 * only keys distributed in this pairing will be used. This includes
	 * STK as it will be replaced by LTK.
	 */
	keys = bt_keys_find_addr(&conn->dst);
	if (keys) {
		bt_keys_clear(keys, BT_KEYS_ALL);
	}

#if defined(CONFIG_BLUETOOTH_CENTRAL)
	/* Slave distributes it's keys first */
	if (conn->role == BT_HCI_ROLE_MASTER && smp->remote_dist) {
		return;
	}
#endif /* CONFIG_BLUETOOTH_CENTRAL */

	bt_smp_distribute_keys(conn);

	/* if all keys were distributed, pairing is done */
	if (!smp->local_dist && !smp->remote_dist) {
		smp_reset(smp);
	}
}

bool bt_smp_irk_matches(const uint8_t irk[16], const bt_addr_t *addr)
{
	uint8_t hash[3];
	int err;

	BT_DBG("IRK %s bdaddr %s", h(irk, 16), bt_addr_str(addr));

	err = smp_ah(irk, addr->val + 3, hash);
	if (err) {
		return false;
	}

	return !memcmp(addr->val, hash, 3);
}

#if defined(CONFIG_BLUETOOTH_SIGNING)
/* 1 bit left shift */
static void array_shift(const uint8_t *in, uint8_t *out)
{
	uint8_t overflow = 0;

	for (int i = 15; i >= 0; i--) {
		out[i] = in[i] << 1;
		/* previous byte */
		out[i] |= overflow;
		overflow = in[i] & 0x80 ? 1 : 0;
	}
}

/* CMAC subkey generation algorithm */
static int cmac_subkey(const uint8_t *key, uint8_t *k1, uint8_t *k2)
{
	const uint8_t rb[16] = {
		[0 ... 14]	= 0x00,
		[15]		= 0x87,
	};
	uint8_t zero[16] = { 0 }, *tmp = zero;
	uint8_t l[16];
	int err;

	/* L := AES-128(K, const_Zero) */
	err = le_encrypt(key, zero, tmp);
	if (err) {
		return err;
	}

	swap_buf(tmp, l, 16);

	BT_DBG("l %s\n", h(l, 16));

	/* if MSB(L) == 0 K1 = L << 1 */
	if (!(l[0] & 0x80)) {
		array_shift(l, k1);
	/* else K1 = (L << 1) XOR rb */
	} else {
		array_shift(l, k1);
		xor_128((uint128_t *)k1, (uint128_t *)rb, (uint128_t *)k1);
	}

	/* if MSB(K1) == 0 K2 = K1 << 1 */
	if (!(k1[0] & 0x80)) {
		array_shift(k1, k2);
	/* else K2 = (K1 << 1) XOR rb */
	} else {
		array_shift(k1, k2);
		xor_128((uint128_t *)k2, (uint128_t *)rb, (uint128_t *)k2);
	}

	return 0;
}

/* padding(x) = x || 10^i      where i is 128 - 8 * r - 1 */
static void add_pad(const uint8_t *in, unsigned char *out, int len)
{
	memset(out, 0, 16);
	memcpy(out, in, len);
	out[len] = 0x80;
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
	uint8_t k1[16], k2[16], last_block[16], *pad_block = last_block;
	uint8_t key_s[16];
	uint8_t *x, *y;
	uint8_t n, flag;
	int err;

	swap_buf(key, key_s, 16);

	/* (K1,K2) = Generate_Subkey(K) */
	err = cmac_subkey(key_s, k1, k2);
	if (err) {
		BT_ERR("SMAC subkey generation failed\n");
		return err;
	}

	BT_DBG("key %s subkeys k1 %s k2 %s\n", h(key, 16), h(k1, 16),
	       h(k2, 16));

	/* the number of blocks, n, is calculated, the block length is 16 bytes
	 * n = ceil(len/const_Bsize)
	 */
	n = (len + 15) / 16;

	/* check input length, flag indicate completed blocks */
	if (n == 0) {
		/* if length is 0, the number of blocks to be processed shall
		 * be 1,and the flag shall be marked as not-complete-block
		 * false
		 */
		n = 1;
		flag = 0;
	} else {
		if ((len % 16) == 0) {
			/* complete blocks */
			flag = 1;
		} else {
			/* last block is not complete */
			flag = 0;
		}
	}

	BT_DBG("len %u n %u flag %u\n", len, n, flag);

	/* if flag is true then M_last = M_n XOR K1 */
	if (flag) {
		xor_128((uint128_t *)&in[16 * (n - 1)], (uint128_t *)k1,
			(uint128_t *)last_block);
	/* else M_last = padding(M_n) XOR K2 */
	} else {
		add_pad(&in[16 * (n - 1)], pad_block, len % 16);
		xor_128((uint128_t *)pad_block, (uint128_t *)k2,
			(uint128_t *)last_block);
	}

	/* Reuse k1 and k2 buffers */
	x = k1;
	y = k2;

	/* Zeroing x */
	memset(x, 0, 16);

	/* the basic CBC-MAC is applied to M_1,...,M_{n-1},M_last */
	for (int i = 0; i < n - 1; i++) {
		/* Y = X XOR M_i */
		xor_128((uint128_t *)x, (uint128_t *)&in[i * 16],
			(uint128_t *)y);

		swap_in_place(y, 16);

		/* X = AES-128(K,Y) */
		err = le_encrypt(key_s, y, x);
		if (err) {
			return err;
		}

		swap_in_place(x, 16);
	}

	/* Y = M_last XOR X */
	xor_128((uint128_t *)x, (uint128_t *)last_block, (uint128_t *)y);

	swap_in_place(y, 16);

	/* T = AES-128(K,Y) */
	err = le_encrypt(key_s, y, out);

	swap_in_place(out, 16);

	return err;
}

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

	BT_DBG("Signing msg %s len %u key %s\n", h(msg, len), len, h(key, 16));

	swap_in_place(m, len + sizeof(cnt));
	swap_buf(key, key_s, 16);

	err = bt_smp_aes_cmac(key_s, m, len + sizeof(cnt), tmp);
	if (err) {
		BT_ERR("Data signing failed\n");
		return err;
	}

	swap_in_place(tmp, sizeof(tmp));
	memcpy(tmp + 4, &cnt, sizeof(cnt));

	/* Swap original message back */
	swap_in_place(m, len + sizeof(cnt));

	memcpy(sig, tmp + 4, 12);

	BT_DBG("sig %s\n", h(sig, 12));

	return 0;
}

int bt_smp_sign_verify(struct bt_conn *conn, struct bt_buf *buf)
{
	struct bt_keys *keys;
	uint8_t sig[12];
	uint32_t cnt;
	int err;

	/* Store signature incl. count */
	memcpy(sig, bt_buf_tail(buf) - sizeof(sig), sizeof(sig));

	keys = bt_keys_get_type(BT_KEYS_REMOTE_CSRK, &conn->dst);
	if (!keys) {
		BT_ERR("Unable to get keys for %s\n",
		       bt_addr_le_str(&conn->dst));
		return -ENOENT;
	}

	/* Copy signing count */
	cnt = sys_cpu_to_le32(keys->remote_csrk.cnt);
	memcpy(bt_buf_tail(buf) - sizeof(sig), &cnt, sizeof(cnt));

	BT_DBG("Sign data len %u key %s count %u\n", buf->len - sizeof(sig),
	       h(keys->remote_csrk.val, 16), keys->remote_csrk.cnt);

	err = smp_sign_buf(keys->remote_csrk.val, buf->data,
			      buf->len - sizeof(sig));
	if (err) {
		BT_ERR("Unable to create signature for %s\n",
		       bt_addr_le_str(&conn->dst));
		return -EIO;
	};

	if (memcmp(sig, bt_buf_tail(buf) - sizeof(sig), sizeof(sig))) {
		BT_ERR("Unable to verify signature for %s\n",
		       bt_addr_le_str(&conn->dst));
		return -EBADMSG;
	};

	keys->remote_csrk.cnt++;

	return 0;
}

int bt_smp_sign(struct bt_conn *conn, struct bt_buf *buf)
{
	struct bt_keys *keys;
	uint32_t cnt;
	int err;

	keys = bt_keys_get_type(BT_KEYS_LOCAL_CSRK, &conn->dst);
	if (!keys) {
		BT_ERR("Unable to get keys for %s\n",
		       bt_addr_le_str(&conn->dst));
		return -ENOENT;
	}

	/* Reserve space for data signature */
	bt_buf_add(buf, 12);

	/* Copy signing count */
	cnt = sys_cpu_to_le32(keys->local_csrk.cnt);
	memcpy(bt_buf_tail(buf) - 12, &cnt, sizeof(cnt));

	BT_DBG("Sign data len %u key %s count %u\n", buf->len,
	       h(keys->local_csrk.val, 16), keys->local_csrk.cnt);

	err = smp_sign_buf(keys->local_csrk.val, buf->data, buf->len - 12);
	if (err) {
		BT_ERR("Unable to create signature for %s\n",
		       bt_addr_le_str(&conn->dst));
		return -EIO;
	};

	keys->local_csrk.cnt++;

	return 0;
}
#else
int bt_smp_sign_verify(struct bt_conn *conn, struct bt_buf *buf)
{
	return -ENOTSUP;
}

int bt_smp_sign(struct bt_conn *conn, struct bt_buf *buf)
{
	return -ENOTSUP;
}
#endif /* CONFIG_BLUETOOTH_SIGNING */

#if defined(CONFIG_BLUETOOTH_SMP_SELFTEST)
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

	BT_DBG("%s: AES CMAC of message with len %u\n", prefix, len);

	bt_smp_aes_cmac(key, m, len, out);
	if (!memcmp(out, mac, 16)) {
		BT_DBG("%s: Success\n", prefix);
	} else {
		BT_ERR("%s: Failed\n", prefix);
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

	BT_DBG("%s: Sign message with len %u\n", prefix, len);

	memset(msg, 0, sizeof(msg));
	memcpy(msg, m, len);
	memset(msg + len, 0, sizeof(uint32_t));

	memcpy(orig, msg, sizeof(msg));

	err = smp_sign_buf(key, msg, len);
	if (err) {
		return err;
	}

	/* Check original message */
	if (!memcmp(msg, orig, len + sizeof(uint32_t))) {
		BT_DBG("%s: Original message intact\n", prefix);
	} else {
		BT_ERR("%s: Original message modified\n", prefix);
		BT_DBG("%s: orig %s\n", prefix, h(orig, sizeof(orig)));
		BT_DBG("%s: msg %s\n", prefix, h(msg, sizeof(msg)));
		return -1;
	}

	if (!memcmp(out, sig, 12)) {
		BT_DBG("%s: Success\n", prefix);
	} else {
		BT_ERR("%s: Failed\n", prefix);
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
	swap_buf(key, key_s, 16);

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

static int smp_self_test(void)
{
	int err;

	err = smp_aes_cmac_test();
	if (err) {
		BT_ERR("SMP AES-CMAC self tests failed\n");
		return err;
	}

	err = smp_sign_test();
	if (err) {
		BT_ERR("SMP signing self tests failed\n");
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

static uint8_t get_io_capa(const struct bt_auth_cb *cb)
{
	if (cb->passkey_display && cb->passkey_entry) {
		return BT_SMP_IO_KEYBOARD_DISPLAY;
	}

	if (cb->passkey_entry) {
		return BT_SMP_IO_KEYBOARD_ONLY;
	}

	if (cb->passkey_display) {
		return BT_SMP_IO_DISPLAY_ONLY;
	}

	return BT_SMP_IO_NO_INPUT_OUTPUT;
}

int bt_auth_cb_register(const struct bt_auth_cb *cb)
{
	if (!cb) {
		bt_smp_io_capa = BT_SMP_IO_NO_INPUT_OUTPUT;
		auth_cb = NULL;
		return 0;
	}

	/* cancel callback should always be provided */
	if (!cb->cancel) {
		return -EINVAL;
	}

	if (auth_cb) {
		return -EALREADY;
	}

	bt_smp_io_capa = get_io_capa(cb);
	auth_cb = cb;

	return 0;
}

void bt_auth_passkey_entry(struct bt_conn *conn, unsigned int passkey)
{
	struct bt_smp *smp = conn->smp;

	passkey = sys_cpu_to_le32(passkey);
	memcpy(smp->tk, &passkey, sizeof(passkey));
	atomic_set_bit(&smp->flags, SMP_FLAG_TK_VALID);

	if (!atomic_test_and_clear_bit(&smp->flags, SMP_FLAG_CFM_DELAYED)) {
		return;
	}

	/* if confirm failed ie. due to invalid passkey, cancel pairing */
	if (smp_send_pairing_confirm(conn) ) {
		bt_auth_cancel(conn);
		return;
	}

	switch(conn->role) {
#if defined(CONFIG_BLUETOOTH_CENTRAL)
	case BT_HCI_ROLE_MASTER:
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PAIRING_CONFIRM);
		break;
#endif /* CONFIG_BLUETOOTH_CENTRAL */

#if defined(CONFIG_BLUETOOTH_PERIPHERAL)
	case BT_HCI_ROLE_SLAVE:
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PAIRING_RANDOM);
		break;
#endif /* CONFIG_BLUETOOTH_PERIPHERAL */
	default:
		break;
	}
}

void bt_auth_cancel(struct bt_conn *conn)
{
	send_err_rsp(conn, BT_SMP_ERR_PASSKEY_ENTRY_FAILED);
	smp_reset(conn->smp);
}

int bt_smp_init(void)
{
	static struct bt_l2cap_chan chan = {
		.cid		= BT_L2CAP_CID_SMP,
		.recv		= bt_smp_recv,
		.connected	= bt_smp_connected,
		.disconnected	= bt_smp_disconnected,
		.encrypt_change	= bt_smp_encrypt_change,
	};

	bt_l2cap_chan_register(&chan);

	return smp_self_test();
}
