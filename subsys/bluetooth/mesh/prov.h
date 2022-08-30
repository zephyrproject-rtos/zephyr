/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2020 Lingao Meng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_MESH_PROV_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_MESH_PROV_H_

#include "prov_bearer.h"

#define PROV_ERR_NONE          0x00
#define PROV_ERR_NVAL_PDU      0x01
#define PROV_ERR_NVAL_FMT      0x02
#define PROV_ERR_UNEXP_PDU     0x03
#define PROV_ERR_CFM_FAILED    0x04
#define PROV_ERR_RESOURCES     0x05
#define PROV_ERR_DECRYPT       0x06
#define PROV_ERR_UNEXP_ERR     0x07
#define PROV_ERR_ADDR          0x08

#define AUTH_METHOD_NO_OOB     0x00
#define AUTH_METHOD_STATIC     0x01
#define AUTH_METHOD_OUTPUT     0x02
#define AUTH_METHOD_INPUT      0x03

#define OUTPUT_OOB_BLINK       0x00
#define OUTPUT_OOB_BEEP        0x01
#define OUTPUT_OOB_VIBRATE     0x02
#define OUTPUT_OOB_NUMBER      0x03
#define OUTPUT_OOB_STRING      0x04

#define INPUT_OOB_PUSH         0x00
#define INPUT_OOB_TWIST        0x01
#define INPUT_OOB_NUMBER       0x02
#define INPUT_OOB_STRING       0x03

#define PUB_KEY_NO_OOB         0x00
#define PUB_KEY_OOB            0x01

#define PROV_INVITE            0x00
#define PROV_CAPABILITIES      0x01
#define PROV_START             0x02
#define PROV_PUB_KEY           0x03
#define PROV_INPUT_COMPLETE    0x04
#define PROV_CONFIRM           0x05
#define PROV_RANDOM            0x06
#define PROV_DATA              0x07
#define PROV_COMPLETE          0x08
#define PROV_FAILED            0x09

#define PROV_NO_PDU            0xff

#define PDU_LEN_INVITE         1
#define PDU_LEN_CAPABILITIES   11
#define PDU_LEN_START          5
#define PDU_LEN_PUB_KEY        64
#define PDU_LEN_INPUT_COMPLETE 0
#define PDU_LEN_CONFIRM        16
#define PDU_LEN_RANDOM         16
#define PDU_LEN_DATA           33
#define PDU_LEN_COMPLETE       0
#define PDU_LEN_FAILED         1

#define PDU_OP_LEN             1

#define PROV_ALG_P256          0x00

#define PROV_IO_OOB_SIZE_MAX   8  /* in bytes */

#define PROV_BUF(name, len) \
	NET_BUF_SIMPLE_DEFINE(name, PROV_BEARER_BUF_HEADROOM + PDU_OP_LEN + len)

enum {
	WAIT_PUB_KEY,           /* Waiting for local PubKey to be generated */
	LINK_ACTIVE,            /* Link has been opened */
	WAIT_NUMBER,            /* Waiting for number input from user */
	WAIT_STRING,            /* Waiting for string input from user */
	NOTIFY_INPUT_COMPLETE,  /* Notify that input has been completed. */
	PROVISIONER,            /* The link was opened as provisioner */
	OOB_PUB_KEY,            /* OOB Public key used */
	PUB_KEY_SENT,           /* Public key has been sent */
	REMOTE_PUB_KEY,         /* Remote key has been received */
	INPUT_COMPLETE,         /* Device input completed */
	WAIT_CONFIRM,           /* Wait for send confirm */
	WAIT_AUTH,              /* Wait for auth response */
	OOB_STATIC_KEY,         /* OOB Static Authentication */
	WAIT_DH_KEY,            /* Wait for DH Key */

	NUM_FLAGS,
};

/** Provisioning role */
struct bt_mesh_prov_role {
	void (*link_opened)(void);

	void (*link_closed)(void);

	void (*error)(uint8_t reason);

	void (*input_complete)(void);

	void (*op[10])(const uint8_t *data);
};

struct bt_mesh_prov_link {
	ATOMIC_DEFINE(flags, NUM_FLAGS);

	const struct prov_bearer *bearer;
	const struct bt_mesh_prov_role *role;

	uint8_t oob_method;             /* Authen method */
	uint8_t oob_action;             /* Authen action */
	uint8_t oob_size;               /* Authen size */
	uint8_t auth[16];               /* Authen value */

	uint8_t dhkey[BT_DH_KEY_LEN];   /* Calculated DHKey */
	uint8_t expect;                 /* Next expected PDU */

	uint8_t conf[16];               /* Local/Remote Confirmation */
	uint8_t rand[16];               /* Local Random */

	uint8_t conf_salt[16];          /* ConfirmationSalt */
	uint8_t conf_key[16];           /* ConfirmationKey */
	/* ConfirmationInput fields: */
	struct {
		uint8_t invite[PDU_LEN_INVITE];
		uint8_t capabilities[PDU_LEN_CAPABILITIES];
		uint8_t start[PDU_LEN_START];
		uint8_t pub_key_provisioner[PDU_LEN_PUB_KEY]; /* big-endian */
		uint8_t pub_key_device[PDU_LEN_PUB_KEY]; /* big-endian */
	} conf_inputs;
	uint8_t prov_salt[16];          /* Provisioning Salt */
};

extern struct bt_mesh_prov_link bt_mesh_prov_link;
extern const struct bt_mesh_prov *bt_mesh_prov;

static inline int bt_mesh_prov_send(struct net_buf_simple *buf,
				    prov_bearer_send_complete_t cb)
{
	return bt_mesh_prov_link.bearer->send(buf, cb, NULL);
}

static inline void bt_mesh_prov_buf_init(struct net_buf_simple *buf, uint8_t type)
{
	net_buf_simple_reserve(buf, PROV_BEARER_BUF_HEADROOM);
	net_buf_simple_add_u8(buf, type);
}

int bt_mesh_prov_reset_state(void (*func)(const uint8_t key[BT_PUB_KEY_LEN]));

bool bt_mesh_prov_active(void);

int bt_mesh_prov_auth(bool is_provisioner, uint8_t method, uint8_t action, uint8_t size);

const struct bt_mesh_prov *bt_mesh_prov_get(void);

void bt_mesh_prov_complete(uint16_t net_idx, uint16_t addr);
void bt_mesh_prov_reset(void);

const struct prov_bearer_cb *bt_mesh_prov_bearer_cb_get(void);

void bt_mesh_pb_adv_recv(struct net_buf_simple *buf);

int bt_mesh_prov_init(const struct bt_mesh_prov *prov);

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_MESH_PROV_H_ */
