/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (c) 2022 T-Mobile USA, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME rsi_bt_smp
#include "rs9116w_ble_smp.h"
#include "rs9116w_ble_keys.h"

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

#define LINK_DIST 0

#define BT_SMP_AUTH_MASK 0x07

#define BT_SMP_AUTH_MASK_SC 0x0f

#define RECV_KEYS                                                              \
	(BT_SMP_DIST_ENC_KEY | BT_SMP_DIST_ID_KEY | SIGN_DIST | LINK_DIST)

#define SEND_KEYS (BT_SMP_DIST_ENC_KEY | ID_DIST | SIGN_DIST | LINK_DIST)

#if defined(CONFIG_BT_BONDABLE)
#define BT_SMP_AUTH_BONDING_FLAGS BT_SMP_AUTH_BONDING
#else
#define BT_SMP_AUTH_BONDING_FLAGS 0
#endif /* CONFIG_BT_BONDABLE */

#define BT_SMP_AUTH_DEFAULT (BT_SMP_AUTH_BONDING_FLAGS | BT_SMP_AUTH_SC)

enum {
	SMP_FLAG_CFM_DELAYED,	/* if confirm should be send when TK is valid */
	SMP_FLAG_ENC_PENDING,	/* if waiting for an encryption change event */
	SMP_FLAG_KEYS_DISTR,	/* if keys distribution phase is in progress */
	SMP_FLAG_PAIRING,	/* if pairing is in progress */
	SMP_FLAG_TIMEOUT,	/* if SMP timeout occurred */
	SMP_FLAG_SC,		/* if LE Secure Connections is used */
	SMP_FLAG_PKEY_SEND,	/* if should send Public Key when available */
	SMP_FLAG_DHKEY_PENDING, /* if waiting for local DHKey */
	SMP_FLAG_DHKEY_GEN,	/* if generating DHKey */
	SMP_FLAG_DHKEY_SEND,	/* if should generate and send DHKey Check */
	SMP_FLAG_USER,		/* if waiting for user input */
	SMP_FLAG_DISPLAY,	/* if display_passkey() callback was called */
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

static bool sc_supported;

enum {
	RSI_EVT_NONE = 0,
	RSI_EVT_SMP_RESP,
	RSI_EVT_PASSKEY,
	RSI_EVT_PASSKEY_DISP,
	RSI_EVT_PASSKEY_DISP_SC,
	RSI_EVT_SMP_FAIL,
	RSI_EVT_ENC_START,
	RSI_EVT_LTK_REQ,
	RSI_EVT_SEC_KEYS
};

struct rsi_smp_fail_s {
	uint8_t dev_addr[6];
	uint16_t status;
};

struct rsi_smp_event_s {
	uint8_t event_type;
	uint16_t status;
	union {
		uint8_t dev_addr[6];
		rsi_bt_event_smp_resp_t smp_resp;
		rsi_bt_event_sc_passkey_t passkey_disp;
		rsi_bt_event_le_ltk_request_t ltk_req;
		rsi_bt_event_encryption_enabled_t enc_start;
		rsi_bt_event_le_security_keys_t le_sec;
	};
};

/* SMP channel specific context */
struct bt_smp {
	/* Commands that remote is allowed to send */
	ATOMIC_DEFINE(allowed_cmds, BT_SMP_NUM_CMDS);

	/* Flags for SMP state machine */
	ATOMIC_DEFINE(flags, SMP_NUM_FLAGS);

	/* Type of method used for pairing */
	uint8_t method;

	/* Pairing Request PDU */
	uint8_t preq[7];

	/* Pairing Response PDU */
	uint8_t prsp[7];

	/* Pairing Confirm PDU */
	uint8_t pcnf[16];

	/* Local random number */
	uint8_t prnd[16];

	/* Remote random number */
	uint8_t rrnd[16];

	/* Temporary key */
	uint8_t tk[16];

	/* Remote Public Key for LE SC */
	uint8_t pkey[64];

	/* DHKey */
	uint8_t dhkey[32];

	/* Remote DHKey check */
	uint8_t e[16];

	/* MacKey */
	uint8_t mackey[16];

	/* LE SC passkey */
	uint32_t passkey;

	/* LE SC passkey round */
	uint8_t passkey_round;

	/* LE SC local OOB data */
	const struct bt_le_oob_sc_data *oobd_local;

	/* LE SC remote OOB data */
	const struct bt_le_oob_sc_data *oobd_remote;

	/* Local key distribution */
	uint8_t local_dist;

	/* Remote key distribution */
	uint8_t remote_dist;

	/* Delayed work for timeout handling */
	struct k_work_delayable work;
};

static struct bt_smp bt_smp_pool[CONFIG_BT_MAX_CONN];

static struct rsi_smp_event_s smp_event_queue[CONFIG_RSI_BT_EVENT_QUEUE_SIZE] = {0};
static int smp_event_ptr;
K_SEM_DEFINE(smp_evt_queue_sem, 1, 1);

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

/* Todo: support fixed passkey */

/**
 * @brief Get the event slot pointer
 *
 * @return Event slot pointer or NULL if none available
 */
static struct rsi_smp_event_s *get_event_slot(void)
{
	k_sem_take(&smp_evt_queue_sem, K_FOREVER);
	int old_event_ptr = smp_event_ptr;

	smp_event_ptr++;
	smp_event_ptr %= CONFIG_RSI_BT_EVENT_QUEUE_SIZE;
	struct rsi_smp_event_s *target_event = &smp_event_queue[smp_event_ptr];

	if (target_event->event_type) {
		smp_event_ptr = old_event_ptr;
		rsi_bt_raise_evt(); /* Raise event to force processing */
		k_sem_give(&smp_evt_queue_sem);
		return NULL;
	}
	k_sem_give(&smp_evt_queue_sem);
	return target_event;
}

/**
 * @brief Callback for smp response event (peripheral mode)
 *
 * @param smp_resp smp response data
 */
void rsi_ble_on_smp_response(rsi_bt_event_smp_resp_t *smp_resp)
{
	BT_DBG("SMP Response Callback");
	struct rsi_smp_event_s *target_evt = get_event_slot();

	if (!target_evt) {
		BT_ERR("Event queue full!");
		return;
	}
	target_evt->event_type = RSI_EVT_SMP_RESP;
	memcpy(&target_evt->smp_resp, smp_resp,
	       sizeof(rsi_bt_event_smp_resp_t));
	rsi_bt_raise_evt();
}

/**
 * @brief Callback to intiate passkey display
 *
 * @param smp_passkey_display Passkey data
 */
void rsi_ble_on_smp_passkey_display(
	rsi_bt_event_smp_passkey_display_t *smp_passkey_display)
{
	BT_DBG("SMP Passkey Display Callback");
	struct rsi_smp_event_s *target_evt = get_event_slot();

	if (!target_evt) {
		BT_ERR("Event queue full!");
		return;
	}
	target_evt->event_type = RSI_EVT_PASSKEY_DISP;
	memcpy(target_evt->passkey_disp.dev_addr, smp_passkey_display->dev_addr,
	       6);
	target_evt->passkey_disp.passkey +=
		(smp_passkey_display->passkey[0] - '0');
	target_evt->passkey_disp.passkey +=
		(smp_passkey_display->passkey[1] - '0') * 10;
	target_evt->passkey_disp.passkey +=
		(smp_passkey_display->passkey[2] - '0') * 100;
	target_evt->passkey_disp.passkey +=
		(smp_passkey_display->passkey[3] - '0') * 1000;
	target_evt->passkey_disp.passkey +=
		(smp_passkey_display->passkey[4] - '0') * 10000;
	target_evt->passkey_disp.passkey +=
		(smp_passkey_display->passkey[5] - '0') * 100000;
	rsi_bt_raise_evt();
}

/**
 * @brief Callback to intiate passkey display (SC)
 *
 * @param sc_passkey Passkey data
 */
void rsi_ble_on_sc_passkey(rsi_bt_event_sc_passkey_t *sc_passkey)
{
	BT_DBG("SMP SC Passkey Callback");
	struct rsi_smp_event_s *target_evt = get_event_slot();

	if (!target_evt) {
		BT_ERR("Event queue full!");
		return;
	}
	target_evt->event_type = RSI_EVT_PASSKEY_DISP_SC;
	memcpy(&target_evt->passkey_disp, sc_passkey,
	       sizeof(rsi_bt_event_sc_passkey_t));
	rsi_bt_raise_evt();
}

/**
 * @brief Callback for smp failure
 *
 * @param status status code
 * @param remote_dev_address address of remote device
 */
void rsi_ble_on_smp_failed(uint16_t status,
			   rsi_bt_event_smp_failed_t *remote_dev_address)
{
	BT_DBG("SMP Failed Callback");
	struct rsi_smp_event_s *target_evt = get_event_slot();

	if (!target_evt) {
		BT_ERR("Event queue full!");
		return;
	}
	target_evt->event_type = RSI_EVT_SMP_FAIL;
	memcpy(target_evt->dev_addr, remote_dev_address->dev_addr, 6);
	target_evt->status = status;
	rsi_bt_raise_evt();
}

/**
 * @brief Callback to indicate encryption start
 *
 * @param status encryption start status code
 * @param enc_enabled encryption data
 */
void rsi_ble_on_encrypt_started(uint16_t status,
				rsi_bt_event_encryption_enabled_t *enc_enabled)
{
	BT_DBG("SMP Encryption Started Callback");
	struct rsi_smp_event_s *target_evt = get_event_slot();

	if (!target_evt) {
		BT_ERR("Event queue full!");
		return;
	}
	target_evt->event_type = RSI_EVT_ENC_START;
	target_evt->status = status;
	memcpy(&target_evt->enc_start, enc_enabled,
	       sizeof(rsi_bt_event_encryption_enabled_t));
	rsi_bt_raise_evt();
}

/**
 * @brief Callback for LTK request
 *
 * @param le_ltk_req LTK request data
 */
static void
rsi_ble_on_le_ltk_req_event(rsi_bt_event_le_ltk_request_t *le_ltk_req)
{
	BT_DBG("SMP LTK Request Callback");
	struct rsi_smp_event_s *target_evt = get_event_slot();

	if (!target_evt) {
		BT_ERR("Event queue full!");
		return;
	}
	target_evt->event_type = RSI_EVT_LTK_REQ;
	memcpy(&target_evt->ltk_req, le_ltk_req,
	       sizeof(rsi_bt_event_le_ltk_request_t));
	rsi_bt_raise_evt();
}

/**
 * @brief Callback for key exchange
 *
 * @param rsi_ble_event_le_security_keys keys + identity address
 */
void rsi_ble_on_le_security_keys(
	rsi_bt_event_le_security_keys_t *rsi_ble_event_le_security_keys)
{
	BT_DBG("SMP Security Keys Callback");
	struct rsi_smp_event_s *target_evt = get_event_slot();

	if (!target_evt) {
		BT_ERR("Event queue full!");
		return;
	}
	target_evt->event_type = RSI_EVT_SEC_KEYS;
	memcpy(&target_evt->le_sec, rsi_ble_event_le_security_keys,
	       sizeof(rsi_bt_event_le_security_keys_t));
	rsi_bt_raise_evt();
}

/**
 * @brief Initializes SMP Callbacks
 *
 */
void bt_smp_init(void)
{
	/* TODO: Deal with legacy pair options/oob */
	sc_supported = true;
	rsi_ble_smp_register_callbacks(
		NULL, rsi_ble_on_smp_response, NULL, rsi_ble_on_smp_failed,
		rsi_ble_on_encrypt_started, rsi_ble_on_smp_passkey_display,
		rsi_ble_on_sc_passkey, rsi_ble_on_le_ltk_req_event,
		rsi_ble_on_le_security_keys, NULL, NULL);
}

/**
 * @brief Get the current IO capability based on available callbacks
 *
 * @return uint8_t IO Capability enum
 */
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
		/* TODO: fix pairing confirm to enable fixed passkey */
		return BT_SMP_IO_KEYBOARD_ONLY;
	}

	if (bt_auth->passkey_display) {
		return BT_SMP_IO_DISPLAY_ONLY;
	}

no_callbacks:
	/* TODO: fix pairing confirm to enable fixed passkey */
	return BT_SMP_IO_NO_INPUT_OUTPUT;
}

static bool bondable = IS_ENABLED(CONFIG_BT_BONDABLE);

/**
 * @brief Extract auth support information
 *
 * @param conn Connection object
 * @param auth Auth flags requested
 * @return uint8_t Corrected auth flags
 */
static uint8_t get_auth(struct bt_conn *conn, uint8_t auth)
{
	if (sc_supported) {
		auth &= BT_SMP_AUTH_MASK_SC;
	} else {
		auth &= BT_SMP_AUTH_MASK;
	}

	if ((get_io_capa() == BT_SMP_IO_NO_INPUT_OUTPUT) ||
	    (!IS_ENABLED(CONFIG_BT_ENFORCE_MITM) &&
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

static rsi_ble_set_smp_pairing_capabilty_data_t pair_cap_data;

/**
 * @brief Initializes pairing (peripheral mode)
 *
 * @param conn Device to initialize pairing with
 * @return int 0 in case of success
 */
static int smp_send_security_req(struct bt_conn *conn)
{
	pair_cap_data.io_capability = get_io_capa();
	pair_cap_data.oob_data_flag = BT_SMP_OOB_NOT_PRESENT;
	pair_cap_data.auth_req = get_auth(conn, BT_SMP_AUTH_DEFAULT);
	pair_cap_data.enc_key_size = BT_SMP_MAX_ENC_KEY_SIZE;
	pair_cap_data.rsp_key_distribution = RECV_KEYS;
	pair_cap_data.ini_key_distribution = SEND_KEYS;
	uint8_t mitm = (pair_cap_data.auth_req & BT_SMP_AUTH_MITM) ? 1 : 0;

	int err = rsi_ble_set_smp_pairing_cap_data(&pair_cap_data);

	printk("Pairing cap data set\n");
	if (err) {
		return err;
	}
	return rsi_ble_smp_pair_request(conn->le.dst.a.val, get_io_capa(),
					mitm);
}

/**
 * @brief Start security: determines appropriate security start function based
 *        on configuration
 *
 * @param conn Device to initialize pairing with
 * @return int 0 in case of success or negative value in case of error
 */
int bt_smp_start_security(struct bt_conn *conn)
{
	switch (conn->role) {
#if defined(CONFIG_BT_PERIPHERAL)
	case BT_HCI_ROLE_SLAVE:
		return smp_send_security_req(conn);
#endif /* CONFIG_BT_PERIPHERAL */
	default:
		return -EINVAL;
	}
}

#if defined(CONFIG_BT_FIXED_PASSKEY)
/**
 * @brief Set passkey
 *
 * @param passkey Passkey
 * @return int 0 in case of success or negative value in case of error
 */
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

/**
 * @brief Send passkey to device
 *
 * @param conn Device to send passkey to
 * @param passkey Passykey
 * @return int 0 in case of success
 */
int bt_smp_auth_passkey_entry(struct bt_conn *conn, unsigned int passkey)
{
	return rsi_ble_smp_passkey(conn->le.dst.a.val, passkey);
}

int bt_smp_auth_passkey_confirm(
	struct bt_conn *conn) /* TODO: Check if this is actually needed */
{
	return -ENOTSUP;
}

#if !defined(CONFIG_BT_SMP_SC_PAIR_ONLY)
int bt_smp_auth_pairing_confirm(struct bt_conn *conn)
{
	/* Not actually needed AFAIK */
	return -ENOTSUP;
}
#else
int bt_smp_auth_pairing_confirm(struct bt_conn *conn)
{
	/* confirm_pairing will never be called in LE SC only mode */
	return -EINVAL;
}

#endif /* !CONFIG_BT_SMP_SC_PAIR_ONLY */

/**
 * @brief Main SMP Processing Loop
 *
 */
void bt_smp_process(void)
{
	k_sem_take(&smp_evt_queue_sem, K_FOREVER);
	struct rsi_smp_event_s *current_event = &smp_event_queue[smp_event_ptr];
	k_sem_give(&smp_evt_queue_sem);

	while (current_event->event_type) {
		BT_DBG("SMP EVT %d", current_event->event_type);
		switch (current_event->event_type) {
		case RSI_EVT_PASSKEY_DISP_SC:
		case RSI_EVT_PASSKEY_DISP: {
			struct bt_conn *selected_conn;

			bt_addr_le_t addr;

			memcpy(addr.a.val, current_event->passkey_disp.dev_addr,
			       6);

			selected_conn = bt_conn_lookup_addr_le(0, &addr);

			if (!selected_conn) {
				BT_WARN("Passkey display event: Unable to find "
					"connection");
				break;
			}

			struct bt_smp *smp =
				&bt_smp_pool[selected_conn->handle];

			smp->passkey = current_event->passkey_disp.passkey;

			if (bt_auth->passkey_display &&
			    current_event->passkey_disp.passkey) {
				bt_auth->passkey_display(
					selected_conn,
					current_event->passkey_disp.passkey);
			} else {
				if (!current_event->passkey_disp.passkey) {
					if (bt_auth->passkey_entry) {
						bt_auth->passkey_entry(
							selected_conn);
					} else {
						BT_WARN("Missing callback to "
							"process passkey");
					}
				} else if (bt_auth->passkey_confirm) {
					bt_auth->passkey_confirm(
						selected_conn,
						current_event->passkey_disp
							.passkey);
				} else {
					BT_WARN("Missing callback to process "
						"passkey");
				}
			}

			if (current_event->passkey_disp.passkey) {
				atomic_set_bit(smp->flags, SMP_FLAG_DISPLAY);
				rsi_ble_smp_passkey(
					current_event->passkey_disp.dev_addr,
					current_event->passkey_disp.passkey);
			}

			bt_conn_unref(selected_conn);
			break;
		}
		case RSI_EVT_SMP_FAIL: {
			struct bt_conn *selected_conn;

			bt_addr_le_t addr;

			memcpy(addr.a.val, current_event->dev_addr, 6);

			selected_conn = bt_conn_lookup_addr_le(0, &addr);

			if (!selected_conn) {
				BT_WARN("SMP fail event: Unable to find "
					"connection");
				break;
			}

			struct bt_smp *smp =
				&bt_smp_pool[selected_conn->handle];
			if (atomic_test_and_clear_bit(smp->flags,
						      SMP_FLAG_USER) ||
			    atomic_test_and_clear_bit(smp->flags,
						      SMP_FLAG_DISPLAY)) {
				if (bt_auth && bt_auth->cancel) {
					bt_auth->cancel(selected_conn);
				}
			}
			if (current_event->status) {
				struct bt_conn_auth_info_cb *listener, *next;
				enum bt_security_err security_err =
					security_err_get(current_event->status &
							 0xFF);
				SYS_SLIST_FOR_EACH_CONTAINER_SAFE(
					&bt_auth_info_cbs, listener, next,
					node) {
					if (listener->pairing_failed) {
						listener->pairing_failed(
							selected_conn,
							security_err);
					}
				}
			}

			bt_conn_unref(selected_conn);
			break;
		}
		case RSI_EVT_ENC_START: {
			struct bt_conn *selected_conn;

			bt_addr_le_t addr;

			memcpy(addr.a.val, current_event->enc_start.dev_addr,
			       6);

			selected_conn = bt_conn_lookup_addr_le(0, &addr);

			if (!selected_conn) {
				BT_WARN("Encrypt start event: Unable to find "
					"connection");
				break;
			}

			struct bt_smp *smp =
				&bt_smp_pool[selected_conn->handle];
			atomic_clear_bit(smp->flags, SMP_FLAG_DISPLAY);
			if (current_event->status) {
				bt_conn_unref(selected_conn);
				break;
			}
			if (!selected_conn->le.keys) {
				selected_conn->le.keys = bt_keys_get_addr(
					0, &selected_conn->le.dst);
			} /* Check if still null */
			if (!selected_conn->le.keys) {
				/* Not sure if this is the proper
				 * solution, however, it should suffice
				 */
				security_changed(selected_conn,
						 BT_SECURITY_ERR_UNSPECIFIED);
				break;
			}
			bool newkey = true;
			uint8_t flags = 0;

			if (!memcmp(current_event->enc_start.localltk,
				    selected_conn->le.keys->ltk.val, 16)) {
				flags = selected_conn->le.keys->flags;
				newkey = false;
			} else {
				if (current_event->enc_start.enabled & BIT(3)) {
					flags = BT_KEYS_AUTHENTICATED |
						BT_KEYS_SC;
				} else if (current_event->enc_start.enabled &
					   BIT(1)) {
					flags = BT_KEYS_AUTHENTICATED;
				}
			}
			bt_security_t old_sec_level = selected_conn->sec_level;

			if (flags & (BT_KEYS_AUTHENTICATED |
				     BT_KEYS_SC)) { /* Todo: ENC KEY SIZE */
				selected_conn->sec_level = BT_SECURITY_L4;
			} else if (flags & BT_KEYS_AUTHENTICATED) {
				selected_conn->sec_level = BT_SECURITY_L3;
			} else if (current_event->enc_start.enabled & BIT(2)) {
				selected_conn->sec_level = BT_SECURITY_L2;
			} else if (current_event->enc_start.enabled & BIT(0)) {
				BT_WARN("Unknown encrypted security "
					"level; Defaulting to L2");
				selected_conn->sec_level = BT_SECURITY_L2;
			} else {
				selected_conn->sec_level = BT_SECURITY_L1;
			}

			if (old_sec_level != selected_conn->sec_level) {
				security_changed(selected_conn,
						 current_event->status);
				struct bt_conn_auth_info_cb *listener, *next;
				enum bt_security_err security_err =
					security_err_get(current_event->status &
							 0xFF);
				SYS_SLIST_FOR_EACH_CONTAINER_SAFE(
					&bt_auth_info_cbs, listener, next,
					node) {
					if (listener->pairing_complete) {
						listener->pairing_complete(
							selected_conn,
							security_err);
					}
				}
			}

			/* Todo: check if theres a more proper way */
			if (newkey) {
				memcpy(selected_conn->le.keys->ltk.val,
				       current_event->enc_start.localltk, 16);
				memcpy(selected_conn->le.keys->ltk.rand,
				       current_event->enc_start.localrand, 8);
				selected_conn->le.keys->flags = flags;
				rsi_uint16_to_2bytes(
					selected_conn->le.keys->ltk.ediv,
					current_event->enc_start.localediv);
			}

			bt_conn_unref(selected_conn);
			break;
		}
		case RSI_EVT_LTK_REQ: {
			/* TODO: Compare to le_ltk_request */
			/* For now */
			struct bt_conn *selected_conn;

			bt_addr_le_t addr;

			memcpy(addr.a.val, current_event->enc_start.dev_addr,
			       6);

			selected_conn = bt_conn_lookup_addr_le(0, &addr);

			if (!selected_conn) {
				BT_WARN("LTK Request: Unable to find "
					"connection");
				break;
			}

			/* First check for keys: */
			if (!selected_conn->le.keys) {
				selected_conn->le.keys = bt_keys_find_addr(
					0, &selected_conn->le.dst);
			}
			/* Verify keys */
			if (selected_conn->le.keys &&
			    (rsi_bytes2R_to_uint16(
				     selected_conn->le.keys->ltk.ediv) ==
			     current_event->ltk_req.localediv) &&
			    !(memcmp(selected_conn->le.keys->ltk.rand,
				     current_event->ltk_req.localrand, 8))) {
				rsi_ble_ltk_req_reply(
					selected_conn->le.dst.a.val, 1,
					selected_conn->le.keys->ltk.val);
			} else {
				rsi_ble_ltk_req_reply(
					selected_conn->le.dst.a.val, 0, NULL);
			}
			bt_conn_unref(selected_conn);
			break;
		}
		case RSI_EVT_SMP_RESP: {
			/* Initiating the pairing process */
			uint8_t mitm = 0;
			bt_addr_le_t addr;

			struct bt_conn *selected_conn;

			memcpy(addr.a.val, current_event->smp_resp.dev_addr, 6);
			selected_conn = bt_conn_lookup_addr_le(0, &addr);

			if (!selected_conn) {
				BT_WARN("SMP response: Unable to find "
					"connection");
				break;
			}

			if ((get_io_capa() != BT_SMP_IO_NO_INPUT_OUTPUT) &&
			    (IS_ENABLED(CONFIG_BT_SMP_ENFORCE_MITM) ||
			     selected_conn->required_sec_level >= 3)) {
				mitm = 1;
			}
			rsi_ble_smp_pair_response(
				current_event->smp_resp.dev_addr, get_io_capa(),
				mitm);

			bt_conn_unref(selected_conn);
			break;
		}
		case RSI_EVT_SEC_KEYS: {
			/* Call identity resolve */
			bt_addr_le_t addr, identity_addr;
			bt_addr_le_t *rpa;

			struct bt_conn *selected_conn;

			memcpy(addr.a.val, current_event->le_sec.dev_addr, 6);
			selected_conn = bt_conn_lookup_addr_le(0, &addr);

			if (!selected_conn) {
				BT_WARN("SMP security keys: Unable to find "
					"connection");
				break;
			}

			identity_addr.type =
				current_event->le_sec.Identity_addr_type;
			memcpy(identity_addr.a.val,
			       current_event->le_sec.Identity_addr, 6);

			if (selected_conn->role == BT_HCI_ROLE_MASTER) {
				rpa = &selected_conn->le.resp_addr;
			} else {
				rpa = &selected_conn->le.init_addr;
			}

			identity_resolved(selected_conn, rpa, &identity_addr);

			bt_conn_unref(selected_conn);

			break;
		}
		default: {
			LOG_ERR("UNHANDLED CASE %d", current_event->event_type);
		}
		}
		k_sem_take(&smp_evt_queue_sem, K_FOREVER);
		current_event->event_type = RSI_EVT_NONE;
		smp_event_ptr--;
		smp_event_ptr = smp_event_ptr < 0 ? ARRAY_SIZE(smp_event_queue)
						  : smp_event_ptr;
		current_event = &smp_event_queue[smp_event_ptr];
		k_sem_give(&smp_evt_queue_sem);
	}
}
