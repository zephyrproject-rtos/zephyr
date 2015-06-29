/**
 * @file smp.c
 * Security Manager Protocol implementation
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <nanokernel.h>
#include <arch/cpu.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <misc/util.h>

#include <bluetooth/log.h>
#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>

#include "hci_core.h"
#include "keys.h"
#include "conn_internal.h"
#include "l2cap.h"
#include "smp.h"

#define RECV_KEYS (BT_SMP_DIST_ID_KEY | BT_SMP_DIST_ENC_KEY)
#define SEND_KEYS (BT_SMP_DIST_ENC_KEY)

/* SMP channel specific context */
struct bt_smp {
	/* The connection this context is associated with */
	struct bt_conn		*conn;

	/* If we're waiting for an encryption change event */
	bool			pending_encrypt;

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
};

static struct bt_smp bt_smp_pool[CONFIG_BLUETOOTH_MAX_CONN];

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
	/* Initialize SMP context */
	memset(smp, 0, sizeof(*smp));

	/* Generate local random number */
	if (le_rand(smp->prnd, 16)) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	BT_DBG("prnd %s\n", h(smp->prnd, 16));

	return 0;
}

static uint8_t smp_pairing_req(struct bt_conn *conn, struct bt_buf *buf)
{
	struct bt_smp_pairing *req = (void *)buf->data;
	struct bt_smp_pairing *rsp;
	struct bt_buf *rsp_buf;
	struct bt_smp *smp = conn->smp;
	uint8_t auth;
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

	/* For JustWorks pairing simplify rsp parameters.
	 * TODO: needs to be reworked later on
	 */
	auth = (req->auth_req & BT_SMP_AUTH_MASK);
	auth &= ~(BT_SMP_AUTH_MITM | BT_SMP_AUTH_SC | BT_SMP_AUTH_KEYPRESS);
	rsp->auth_req = auth;
	rsp->io_capability = BT_SMP_IO_NO_INPUT_OUTPUT;
	rsp->oob_flag = BT_SMP_OOB_NOT_PRESENT;
	rsp->max_key_size = req->max_key_size;
	rsp->init_key_dist = (req->init_key_dist & RECV_KEYS);
	rsp->resp_key_dist = (req->resp_key_dist & SEND_KEYS);

	smp->local_dist = rsp->resp_key_dist;

	memset(smp->tk, 0, sizeof(smp->tk));

	/* Store req/rsp for later use */
	smp->preq[0] = BT_SMP_CMD_PAIRING_REQ;
	memcpy(smp->preq + 1, req, sizeof(*req));
	smp->prsp[0] = BT_SMP_CMD_PAIRING_RSP;
	memcpy(smp->prsp + 1, rsp, sizeof(*rsp));

	bt_l2cap_send(conn, BT_L2CAP_CID_SMP, rsp_buf);

	return 0;
}

int smp_send_pairing_req(struct bt_conn *conn)
{
	struct bt_smp *smp = conn->smp;
	struct bt_smp_pairing *req;
	struct bt_buf *req_buf;

	BT_DBG("\n");

	if (smp_init(smp)) {
		return -ENOBUFS;
	}

	req_buf = bt_smp_create_pdu(conn, BT_SMP_CMD_PAIRING_REQ, sizeof(*req));
	if (!req_buf) {
		return -ENOBUFS;
	}

	req = bt_buf_add(req_buf, sizeof(*req));

	/* For JustWorks pairing simplify req parameters.
	 * TODO: needs to be reworked later on
	 */
	req->auth_req = BT_SMP_AUTH_BONDING;
	req->io_capability = BT_SMP_IO_NO_INPUT_OUTPUT;
	req->oob_flag = BT_SMP_OOB_NOT_PRESENT;
	req->max_key_size = BT_SMP_MAX_ENC_KEY_SIZE;
	req->init_key_dist = SEND_KEYS;
	req->resp_key_dist = RECV_KEYS;

	smp->local_dist = SEND_KEYS;

	memset(smp->tk, 0, sizeof(smp->tk));

	/* Store req for later use */
	smp->preq[0] = BT_SMP_CMD_PAIRING_REQ;
	memcpy(smp->preq + 1, req, sizeof(*req));

	bt_l2cap_send(conn, BT_L2CAP_CID_SMP, req_buf);

	return 0;
}

static uint8_t smp_send_pairing_confirm(struct bt_conn *conn)
{
	struct bt_smp_pairing_confirm *req;
	struct bt_smp *smp = conn->smp;
	const bt_addr_le_t *ra, *ia;
	struct bt_buf *rsp_buf;
	int err;

	rsp_buf = bt_smp_create_pdu(conn, BT_SMP_CMD_PAIRING_CONFIRM,
				    sizeof(*req));
	if (!rsp_buf) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	req = bt_buf_add(rsp_buf, sizeof(*req));

	if (conn->role == BT_HCI_ROLE_MASTER) {
		ra = &conn->dst;
		ia = &conn->src;
	} else {
		ra = &conn->src;
		ia = &conn->dst;
	}

	err = smp_c1(smp->tk, smp->prnd, smp->preq, smp->prsp, ia, ra,
		     req->val);
	if (err) {
		bt_buf_put(rsp_buf);
		return BT_SMP_ERR_UNSPECIFIED;
	}

	bt_l2cap_send(conn, BT_L2CAP_CID_SMP, rsp_buf);

	return 0;
}

static uint8_t smp_pairing_rsp(struct bt_conn *conn, struct bt_buf *buf)
{
	struct bt_smp_pairing *rsp = (void *)buf->data;
	struct bt_smp *smp = conn->smp;

	BT_DBG("\n");

	if ((rsp->max_key_size > BT_SMP_MAX_ENC_KEY_SIZE) ||
	    (rsp->max_key_size < BT_SMP_MIN_ENC_KEY_SIZE)) {
		return BT_SMP_ERR_ENC_KEY_SIZE;
	}

	smp->local_dist &= rsp->resp_key_dist;

	/* Store rsp for later use */
	smp->prsp[0] = BT_SMP_CMD_PAIRING_RSP;
	memcpy(smp->prsp + 1, rsp, sizeof(*rsp));

	return smp_send_pairing_confirm(conn);
}

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

	return 0;
}

static uint8_t smp_pairing_confirm(struct bt_conn *conn, struct bt_buf *buf)
{
	struct bt_smp_pairing_confirm *req = (void *)buf->data;
	struct bt_smp *smp = conn->smp;

	BT_DBG("\n");

	memcpy(smp->pcnf, req->val, sizeof(smp->pcnf));

	if (conn->role == BT_HCI_ROLE_SLAVE) {
		return smp_send_pairing_confirm(conn);
	}

	return smp_send_pairing_random(conn);
}

static uint8_t smp_pairing_random(struct bt_conn *conn, struct bt_buf *buf)
{
	struct bt_smp_pairing_random *req = (void *)buf->data;
	const bt_addr_le_t *ra, *ia;
	struct bt_smp *smp = conn->smp;
	struct bt_keys *keys;
	uint8_t cfm[16];
	int err;

	BT_DBG("\n");

	memcpy(smp->rrnd, req->val, sizeof(smp->rrnd));

	if (conn->role == BT_HCI_ROLE_MASTER) {
		ra = &conn->dst;
		ia = &conn->src;
	} else {
		ra = &conn->src;
		ia = &conn->dst;
	}

	err = smp_c1(smp->tk, smp->rrnd, smp->preq, smp->prsp, ia, ra, cfm);
	if (err) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	BT_DBG("pcnf %s cfm %s\n", h(smp->pcnf, 16), h(cfm, 16));

	if (memcmp(smp->pcnf, cfm, sizeof(smp->pcnf))) {
		return BT_SMP_ERR_CONFIRM_FAILED;
	}

	if (conn->role == BT_HCI_ROLE_MASTER) {
		uint8_t stk[16];

		/* No need to store master STK */
		err = smp_s1(smp->tk, smp->rrnd, smp->prnd, stk);
		if (err) {
			return BT_SMP_ERR_UNSPECIFIED;
		}

		/* Rand and EDiv are 0 for the STK */
		if (bt_hci_le_start_encryption(conn->handle, 0, 0, stk)) {
			BT_ERR("Failed to start encryption\n");
			return BT_SMP_ERR_UNSPECIFIED;
		}

		smp->pending_encrypt = true;

		return 0;
	}

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

	/* Rand and EDiv are 0 for the STK */
	keys->slave_ltk.rand = 0;
	keys->slave_ltk.ediv = 0;

	BT_DBG("generated STK %s\n", h(keys->slave_ltk.val, 16));

	smp->pending_encrypt = true;

	smp_send_pairing_random(conn);

	return 0;
}

static uint8_t smp_ident_info(struct bt_conn *conn, struct bt_buf *buf)
{
	struct bt_smp_ident_info *req = (void *)buf->data;
	struct bt_keys *keys;

	BT_DBG("\n");

	keys = bt_keys_get_type(BT_KEYS_IRK, &conn->dst);
	if (!keys) {
		BT_ERR("Unable to get keys for %s\n",
		       bt_addr_le_str(&conn->dst));
		return BT_SMP_ERR_UNSPECIFIED;
	}

	memcpy(keys->irk.val, req->irk, 16);

	return 0;
}

static uint8_t smp_ident_addr_info(struct bt_conn *conn, struct bt_buf *buf)
{
	struct bt_smp_ident_addr_info *req = (void *)buf->data;
	struct bt_keys *keys;

	BT_DBG("\n");

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

	if (bt_addr_le_is_rpa(&conn->dst)) {
		bt_addr_copy(&keys->irk.rpa, (bt_addr_t *)&conn->dst.val);
		bt_addr_le_copy(&keys->addr, &req->addr);
		bt_addr_le_copy(&conn->dst, &req->addr);
	}

	return 0;
}

static const struct {
	uint8_t  (*func)(struct bt_conn *conn, struct bt_buf *buf);
	uint8_t  expect_len;
} handlers[] = {
	{ }, /* No op-code defined for 0x00 */
	{ smp_pairing_req,         sizeof(struct bt_smp_pairing) },
	{ smp_pairing_rsp,         sizeof(struct bt_smp_pairing) },
	{ smp_pairing_confirm,     sizeof(struct bt_smp_pairing_confirm) },
	{ smp_pairing_random,      sizeof(struct bt_smp_pairing_random) },
	{ }, /* Pairing Failed - Not yet implemented */
	{ }, /* Encrypt Information - Not yet implemented */
	{ }, /* Master Identification - Not yet implemented */
	{ smp_ident_info,          sizeof(struct bt_smp_ident_info) },
	{ smp_ident_addr_info,     sizeof(struct bt_smp_ident_addr_info) },
};

static void bt_smp_recv(struct bt_conn *conn, struct bt_buf *buf)
{
	struct bt_smp_hdr *hdr = (void *)buf->data;
	uint8_t err;

	if (buf->len < sizeof(*hdr)) {
		BT_ERR("Too small SMP PDU received\n");
		goto done;
	}

	BT_DBG("Received SMP code 0x%02x len %u\n", hdr->code, buf->len);

	bt_buf_pull(buf, sizeof(*hdr));

	if (hdr->code >= ARRAY_SIZE(handlers) || !handlers[hdr->code].func) {
		BT_WARN("Unhandled SMP code 0x%02x\n", hdr->code);
		err = BT_SMP_ERR_CMD_NOTSUPP;
	} else {
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

		if (!smp->conn) {
			smp->conn = conn;
			conn->smp = smp;
			return;
		}
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
	memset(smp, 0, sizeof(*smp));
}

static void bt_smp_encrypt_change(struct bt_conn *conn)
{
	struct bt_smp *smp = conn->smp;
	struct bt_keys *keys;
	struct bt_buf *buf;

	BT_DBG("conn %p handle %u encrypt 0x%02x\n", conn, conn->handle,
	        conn->encrypt);

	if (!smp || !conn->encrypt) {
		return;
	}

	if (!smp->pending_encrypt) {
		return;
	}

	smp->pending_encrypt = false;

	keys = bt_keys_get_addr(&conn->dst);
	if (!keys) {
		BT_ERR("Unable to look up keys for %s\n",
		       bt_addr_le_str(&conn->dst));
		return;
	}

	if (!smp->local_dist) {
		bt_keys_clear(keys, BT_KEYS_ALL);
		return;
	}

	if (smp->local_dist & BT_SMP_DIST_ENC_KEY) {
		struct bt_smp_encrypt_info *info;
		struct bt_smp_master_ident *ident;

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
		memcpy(info->ltk, keys->slave_ltk.val, sizeof(info->ltk));

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

void bt_smp_init(void)
{
	static struct bt_l2cap_chan chan = {
		.cid		= BT_L2CAP_CID_SMP,
		.recv		= bt_smp_recv,
		.connected	= bt_smp_connected,
		.disconnected	= bt_smp_disconnected,
		.encrypt_change	= bt_smp_encrypt_change,
	};

	bt_l2cap_chan_register(&chan);
}
