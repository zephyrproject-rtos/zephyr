/* gatt.c - Generic Attribute Profile handling */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/atomic.h>
#include <sys/byteorder.h>
#include <sys/util.h>

#include <settings/settings.h>

#if defined(CONFIG_BT_GATT_CACHING)
#include <tinycrypt/constants.h>
#include <tinycrypt/utils.h>
#include <tinycrypt/aes.h>
#include <tinycrypt/cmac_mode.h>
#include <tinycrypt/ccm_mode.h>
#endif /* CONFIG_BT_GATT_CACHING */

#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <drivers/bluetooth/hci_driver.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_GATT)
#define LOG_MODULE_NAME bt_gatt
#include "common/log.h"

#include "hci_core.h"
#include "conn_internal.h"
#include "keys.h"
#include "l2cap_internal.h"
#include "att_internal.h"
#include "smp.h"
#include "settings.h"
#include "gatt_internal.h"

#define SC_TIMEOUT	K_MSEC(10)
#define CCC_STORE_DELAY	K_SECONDS(1)

#define DB_HASH_TIMEOUT	K_MSEC(10)

static uint16_t last_static_handle;

/* Persistent storage format for GATT CCC */
struct ccc_store {
	uint16_t handle;
	uint16_t value;
};

struct gatt_sub {
	uint8_t id;
	bt_addr_le_t peer;
	sys_slist_t list;
};

#if defined(CONFIG_BT_GATT_CLIENT)
#define SUB_MAX (CONFIG_BT_MAX_PAIRED + CONFIG_BT_MAX_CONN)
#else
#define SUB_MAX 0
#endif /* CONFIG_BT_GATT_CLIENT */

static struct gatt_sub subscriptions[SUB_MAX];

static const uint16_t gap_appearance = CONFIG_BT_DEVICE_APPEARANCE;

#if defined(CONFIG_BT_GATT_DYNAMIC_DB)
static sys_slist_t db;
#endif /* CONFIG_BT_GATT_DYNAMIC_DB */

static atomic_t init;

static ssize_t read_name(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	const char *name = bt_get_name();

	return bt_gatt_attr_read(conn, attr, buf, len, offset, name,
				 strlen(name));
}

#if defined(CONFIG_BT_DEVICE_NAME_GATT_WRITABLE)

static ssize_t write_name(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, uint16_t len, uint16_t offset,
			 uint8_t flags)
{
	char value[CONFIG_BT_DEVICE_NAME_MAX] = {};

	if (offset) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len >= sizeof(value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(value, buf, len);

	bt_set_name(value);

	return len;
}

#endif /* CONFIG_BT_DEVICE_NAME_GATT_WRITABLE */

static ssize_t read_appearance(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	uint16_t appearance = sys_cpu_to_le16(gap_appearance);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &appearance,
				 sizeof(appearance));
}

#if defined (CONFIG_BT_GAP_PERIPHERAL_PREF_PARAMS)
/* This checks if the range entered is valid */
BUILD_ASSERT(!(CONFIG_BT_PERIPHERAL_PREF_MIN_INT > 3200 &&
	     CONFIG_BT_PERIPHERAL_PREF_MIN_INT < 0xffff));
BUILD_ASSERT(!(CONFIG_BT_PERIPHERAL_PREF_MAX_INT > 3200 &&
	     CONFIG_BT_PERIPHERAL_PREF_MAX_INT < 0xffff));
BUILD_ASSERT(!(CONFIG_BT_PERIPHERAL_PREF_TIMEOUT > 3200 &&
	     CONFIG_BT_PERIPHERAL_PREF_TIMEOUT < 0xffff));
BUILD_ASSERT((CONFIG_BT_PERIPHERAL_PREF_MIN_INT == 0xffff) ||
	     (CONFIG_BT_PERIPHERAL_PREF_MIN_INT <=
	     CONFIG_BT_PERIPHERAL_PREF_MAX_INT));

static ssize_t read_ppcp(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	struct __packed {
		uint16_t min_int;
		uint16_t max_int;
		uint16_t latency;
		uint16_t timeout;
	} ppcp;

	ppcp.min_int = sys_cpu_to_le16(CONFIG_BT_PERIPHERAL_PREF_MIN_INT);
	ppcp.max_int = sys_cpu_to_le16(CONFIG_BT_PERIPHERAL_PREF_MAX_INT);
	ppcp.latency = sys_cpu_to_le16(CONFIG_BT_PERIPHERAL_PREF_SLAVE_LATENCY);
	ppcp.timeout = sys_cpu_to_le16(CONFIG_BT_PERIPHERAL_PREF_TIMEOUT);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &ppcp,
				 sizeof(ppcp));
}
#endif

#if defined(CONFIG_BT_CENTRAL) && defined(CONFIG_BT_PRIVACY)
static ssize_t read_central_addr_res(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr, void *buf,
				     uint16_t len, uint16_t offset)
{
	uint8_t central_addr_res = BT_GATT_CENTRAL_ADDR_RES_SUPP;

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &central_addr_res, sizeof(central_addr_res));
}
#endif /* CONFIG_BT_CENTRAL && CONFIG_BT_PRIVACY */

BT_GATT_SERVICE_DEFINE(_2_gap_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_GAP),
#if defined(CONFIG_BT_DEVICE_NAME_GATT_WRITABLE)
	/* Require pairing for writes to device name */
	BT_GATT_CHARACTERISTIC(BT_UUID_GAP_DEVICE_NAME,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT,
			       read_name, write_name, bt_dev.name),
#else
	BT_GATT_CHARACTERISTIC(BT_UUID_GAP_DEVICE_NAME, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_name, NULL, NULL),
#endif /* CONFIG_BT_DEVICE_NAME_GATT_WRITABLE */
	BT_GATT_CHARACTERISTIC(BT_UUID_GAP_APPEARANCE, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_appearance, NULL, NULL),
#if defined(CONFIG_BT_CENTRAL) && defined(CONFIG_BT_PRIVACY)
	BT_GATT_CHARACTERISTIC(BT_UUID_CENTRAL_ADDR_RES,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       read_central_addr_res, NULL, NULL),
#endif /* CONFIG_BT_CENTRAL && CONFIG_BT_PRIVACY */
#if defined(CONFIG_BT_GAP_PERIPHERAL_PREF_PARAMS)
	BT_GATT_CHARACTERISTIC(BT_UUID_GAP_PPCP, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_ppcp, NULL, NULL),
#endif
);

struct sc_data {
	uint16_t start;
	uint16_t end;
} __packed;

struct gatt_sc_cfg {
	uint8_t		id;
	bt_addr_le_t	peer;
	struct {
		uint16_t		start;
		uint16_t		end;
	} data;
};

#define SC_CFG_MAX (CONFIG_BT_MAX_PAIRED + CONFIG_BT_MAX_CONN)
static struct gatt_sc_cfg sc_cfg[SC_CFG_MAX];
BUILD_ASSERT(sizeof(struct sc_data) == sizeof(sc_cfg[0].data));

static struct gatt_sc_cfg *find_sc_cfg(uint8_t id, bt_addr_le_t *addr)
{
	BT_DBG("id: %u, addr: %s", id, bt_addr_le_str(addr));

	for (size_t i = 0; i < ARRAY_SIZE(sc_cfg); i++) {
		if (id == sc_cfg[i].id &&
		    !bt_addr_le_cmp(&sc_cfg[i].peer, addr)) {
			return &sc_cfg[i];
		}
	}

	return NULL;
}

static void sc_store(struct gatt_sc_cfg *cfg)
{
	char key[BT_SETTINGS_KEY_MAX];
	int err;

	if (cfg->id) {
		char id_str[4];

		u8_to_dec(id_str, sizeof(id_str), cfg->id);
		bt_settings_encode_key(key, sizeof(key), "sc",
				       &cfg->peer, id_str);
	} else {
		bt_settings_encode_key(key, sizeof(key), "sc",
				       &cfg->peer, NULL);
	}

	err = settings_save_one(key, (char *)&cfg->data, sizeof(cfg->data));
	if (err) {
		BT_ERR("failed to store SC (err %d)", err);
		return;
	}

	BT_DBG("stored SC for %s (%s, 0x%04x-0x%04x)",
	       bt_addr_le_str(&cfg->peer), log_strdup(key), cfg->data.start,
	       cfg->data.end);
}

static void clear_sc_cfg(struct gatt_sc_cfg *cfg)
{
	memset(cfg, 0, sizeof(*cfg));
}

static int bt_gatt_clear_sc(uint8_t id, const bt_addr_le_t *addr)
{

	struct gatt_sc_cfg *cfg;

	cfg = find_sc_cfg(id, (bt_addr_le_t *)addr);
	if (!cfg) {
		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		char key[BT_SETTINGS_KEY_MAX];
		int err;

		if (cfg->id) {
			char id_str[4];

			u8_to_dec(id_str, sizeof(id_str), cfg->id);
			bt_settings_encode_key(key, sizeof(key), "sc",
					       &cfg->peer, id_str);
		} else {
			bt_settings_encode_key(key, sizeof(key), "sc",
					       &cfg->peer, NULL);
		}

		err = settings_delete(key);
		if (err) {
			BT_ERR("failed to delete SC (err %d)", err);
		} else {
			BT_DBG("deleted SC for %s (%s)",
			       bt_addr_le_str(&cfg->peer),
			       log_strdup(key));
		}
	}

	clear_sc_cfg(cfg);

	return 0;
}

static void sc_clear(struct bt_conn *conn)
{
	if (bt_addr_le_is_bonded(conn->id, &conn->le.dst)) {
		int err;

		err = bt_gatt_clear_sc(conn->id, &conn->le.dst);
		if (err) {
			BT_ERR("Failed to clear SC %d", err);
		}
	} else {
		struct gatt_sc_cfg *cfg;

		cfg = find_sc_cfg(conn->id, &conn->le.dst);
		if (cfg) {
			clear_sc_cfg(cfg);
		}
	}
}

static void sc_reset(struct gatt_sc_cfg *cfg)
{
	BT_DBG("peer %s", bt_addr_le_str(&cfg->peer));

	memset(&cfg->data, 0, sizeof(cfg->data));

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		sc_store(cfg);
	}
}

static bool update_range(uint16_t *start, uint16_t *end, uint16_t new_start,
			 uint16_t new_end)
{
	BT_DBG("start 0x%04x end 0x%04x new_start 0x%04x new_end 0x%04x",
	       *start, *end, new_start, new_end);

	/* Check if inside existing range */
	if (new_start >= *start && new_end <= *end) {
		return false;
	}

	/* Update range */
	if (*start > new_start) {
		*start = new_start;
	}

	if (*end < new_end) {
		*end = new_end;
	}

	return true;
}

static void sc_save(uint8_t id, bt_addr_le_t *peer, uint16_t start, uint16_t end)
{
	struct gatt_sc_cfg *cfg;
	bool modified = false;

	BT_DBG("peer %s start 0x%04x end 0x%04x", bt_addr_le_str(peer), start,
	       end);

	cfg = find_sc_cfg(id, peer);
	if (!cfg) {
		/* Find and initialize a free sc_cfg entry */
		cfg = find_sc_cfg(BT_ID_DEFAULT, BT_ADDR_LE_ANY);
		if (!cfg) {
			BT_ERR("unable to save SC: no cfg left");
			return;
		}

		cfg->id = id;
		bt_addr_le_copy(&cfg->peer, peer);
	}

	/* Check if there is any change stored */
	if (!(cfg->data.start || cfg->data.end)) {
		cfg->data.start = start;
		cfg->data.end = end;
		modified = true;
		goto done;
	}

	modified = update_range(&cfg->data.start, &cfg->data.end, start, end);

done:
	if (IS_ENABLED(CONFIG_BT_SETTINGS) &&
	    modified && bt_addr_le_is_bonded(cfg->id, &cfg->peer)) {
		sc_store(cfg);
	}
}

static ssize_t sc_ccc_cfg_write(struct bt_conn *conn,
				const struct bt_gatt_attr *attr, uint16_t value)
{
	BT_DBG("value 0x%04x", value);

	if (value == BT_GATT_CCC_INDICATE) {
		/* Create a new SC configuration entry if subscribed */
		sc_save(conn->id, &conn->le.dst, 0, 0);
	} else {
		sc_clear(conn);
	}

	return sizeof(value);
}

static struct _bt_gatt_ccc sc_ccc = BT_GATT_CCC_INITIALIZER(NULL,
							    sc_ccc_cfg_write,
							    NULL);

enum {
	CF_CHANGE_AWARE,	/* Client is changed aware */
	CF_OUT_OF_SYNC,		/* Client is out of sync */

	/* Total number of flags - must be at the end of the enum */
	CF_NUM_FLAGS,
};

#define CF_BIT_ROBUST_CACHING	0
#define CF_BIT_EATT		1
#define CF_BIT_NOTIFY_MULTI	2
#define CF_BIT_LAST		CF_BIT_NOTIFY_MULTI

#define CF_BYTE_LAST		(CF_BIT_LAST % 8)

#define CF_ROBUST_CACHING(_cfg) (_cfg->data[0] & BIT(CF_BIT_ROBUST_CACHING))
#define CF_EATT(_cfg) (_cfg->data[0] & BIT(CF_BIT_EATT))
#define CF_NOTIFY_MULTI(_cfg) (_cfg->data[0] & BIT(CF_BIT_NOTIFY_MULTI))

struct gatt_cf_cfg {
	uint8_t                    id;
	bt_addr_le_t		peer;
	uint8_t			data[1];
	ATOMIC_DEFINE(flags, CF_NUM_FLAGS);
};

#if defined(CONFIG_BT_GATT_CACHING)
#define CF_CFG_MAX (CONFIG_BT_MAX_PAIRED + CONFIG_BT_MAX_CONN)
#else
#define CF_CFG_MAX 0
#endif /* CONFIG_BT_GATT_CACHING */

static struct gatt_cf_cfg cf_cfg[CF_CFG_MAX] = {};

static void clear_cf_cfg(struct gatt_cf_cfg *cfg)
{
	bt_addr_le_copy(&cfg->peer, BT_ADDR_LE_ANY);
	memset(cfg->data, 0, sizeof(cfg->data));
	atomic_set(cfg->flags, 0);
}

#if defined(CONFIG_BT_GATT_CACHING)
static struct gatt_cf_cfg *find_cf_cfg(struct bt_conn *conn)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(cf_cfg); i++) {
		struct gatt_cf_cfg *cfg = &cf_cfg[i];

		if (!conn) {
			if (!bt_addr_le_cmp(&cfg->peer, BT_ADDR_LE_ANY)) {
				return cfg;
			}
		} else if (bt_conn_is_peer_addr_le(conn, cfg->id, &cfg->peer)) {
			return cfg;
		}
	}

	return NULL;
}

static ssize_t cf_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		       void *buf, uint16_t len, uint16_t offset)
{
	struct gatt_cf_cfg *cfg;
	uint8_t data[1] = {};

	cfg = find_cf_cfg(conn);
	if (cfg) {
		memcpy(data, cfg->data, sizeof(data));
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, data,
				 sizeof(data));
}

static bool cf_set_value(struct gatt_cf_cfg *cfg, const uint8_t *value, uint16_t len)
{
	uint16_t i;
	uint8_t last_byte = CF_BYTE_LAST;
	uint8_t last_bit = CF_BIT_LAST;

	/* Validate the bits */
	for (i = 0U; i < len && i <= last_byte; i++) {
		uint8_t chg_bits = value[i] ^ cfg->data[i];
		uint8_t bit;

		for (bit = 0U; bit <= last_bit; bit++) {
			/* A client shall never clear a bit it has set */
			if ((BIT(bit) & chg_bits) &&
			    (BIT(bit) & cfg->data[i])) {
				return false;
			}
		}
	}

	/* Set the bits for each octect */
	for (i = 0U; i < len && i < last_byte; i++) {
		cfg->data[i] |= value[i] & (BIT(last_bit + 1) - 1);
		BT_DBG("byte %u: data 0x%02x value 0x%02x", i, cfg->data[i],
		       value[i]);
	}

	return true;
}

static ssize_t cf_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	struct gatt_cf_cfg *cfg;
	const uint8_t *value = buf;

	if (offset > sizeof(cfg->data)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (offset + len > sizeof(cfg->data)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	cfg = find_cf_cfg(conn);
	if (!cfg) {
		cfg = find_cf_cfg(NULL);
	}

	if (!cfg) {
		BT_WARN("No space to store Client Supported Features");
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	BT_DBG("handle 0x%04x len %u", attr->handle, len);

	if (!cf_set_value(cfg, value, len)) {
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	bt_addr_le_copy(&cfg->peer, &conn->le.dst);
	cfg->id = conn->id;
	atomic_set_bit(cfg->flags, CF_CHANGE_AWARE);

	return len;
}

static uint8_t db_hash[16];
struct k_delayed_work db_hash_work;

struct gen_hash_state {
	struct tc_cmac_struct state;
	int err;
};

static uint8_t gen_hash_m(const struct bt_gatt_attr *attr, uint16_t handle,
			  void *user_data)
{
	struct gen_hash_state *state = user_data;
	struct bt_uuid_16 *u16;
	uint8_t data[16];
	ssize_t len;
	uint16_t value;

	if (attr->uuid->type != BT_UUID_TYPE_16)
		return BT_GATT_ITER_CONTINUE;

	u16 = (struct bt_uuid_16 *)attr->uuid;

	switch (u16->val) {
	/* Attributes to hash: handle + UUID + value */
	case 0x2800: /* GATT Primary Service */
	case 0x2801: /* GATT Secondary Service */
	case 0x2802: /* GATT Include Service */
	case 0x2803: /* GATT Characteristic */
	case 0x2900: /* GATT Characteristic Extended Properties */
		value = sys_cpu_to_le16(handle);
		if (tc_cmac_update(&state->state, (uint8_t *)&value,
				   sizeof(handle)) == TC_CRYPTO_FAIL) {
			state->err = -EINVAL;
			return BT_GATT_ITER_STOP;
		}

		value = sys_cpu_to_le16(u16->val);
		if (tc_cmac_update(&state->state, (uint8_t *)&value,
				   sizeof(u16->val)) == TC_CRYPTO_FAIL) {
			state->err = -EINVAL;
			return BT_GATT_ITER_STOP;
		}

		len = attr->read(NULL, attr, data, sizeof(data), 0);
		if (len < 0) {
			state->err = len;
			return BT_GATT_ITER_STOP;
		}

		if (tc_cmac_update(&state->state, data, len) ==
		    TC_CRYPTO_FAIL) {
			state->err = -EINVAL;
			return BT_GATT_ITER_STOP;
		}

		break;
	/* Attributes to hash: handle + UUID */
	case 0x2901: /* GATT Characteristic User Descriptor */
	case 0x2902: /* GATT Client Characteristic Configuration */
	case 0x2903: /* GATT Server Characteristic Configuration */
	case 0x2904: /* GATT Characteristic Presentation Format */
	case 0x2905: /* GATT Characteristic Aggregated Format */
		value = sys_cpu_to_le16(handle);
		if (tc_cmac_update(&state->state, (uint8_t *)&value,
				   sizeof(handle)) == TC_CRYPTO_FAIL) {
			state->err = -EINVAL;
			return BT_GATT_ITER_STOP;
		}

		value = sys_cpu_to_le16(u16->val);
		if (tc_cmac_update(&state->state, (uint8_t *)&value,
				   sizeof(u16->val)) == TC_CRYPTO_FAIL) {
			state->err = -EINVAL;
			return BT_GATT_ITER_STOP;
		}
		break;
	default:
		return BT_GATT_ITER_CONTINUE;
	}

	return BT_GATT_ITER_CONTINUE;
}

static void db_hash_store(void)
{
	int err;

	err = settings_save_one("bt/hash", &db_hash, sizeof(db_hash));
	if (err) {
		BT_ERR("Failed to save Database Hash (err %d)", err);
	}

	BT_DBG("Database Hash stored");
}

/* Once the db_hash work has started we cannot cancel it anymore, so the
 * assumption is made that the in-progress work cannot be pre-empted.
 * This assumption should hold as long as calculation does not make any calls
 * that would make it unready.
 * If this assumption is no longer true we will have to solve the case where
 * k_delayed_work_cancel failed because the work was in-progress but pre-empted.
 */
static void db_hash_gen(bool store)
{
	uint8_t key[16] = {};
	struct tc_aes_key_sched_struct sched;
	struct gen_hash_state state;

	if (tc_cmac_setup(&state.state, key, &sched) == TC_CRYPTO_FAIL) {
		BT_ERR("Unable to setup AES CMAC");
		return;
	}

	bt_gatt_foreach_attr(0x0001, 0xffff, gen_hash_m, &state);

	if (tc_cmac_final(db_hash, &state.state) == TC_CRYPTO_FAIL) {
		BT_ERR("Unable to calculate hash");
		return;
	}

	/**
	 * Core 5.1 does not state the endianess of the hash.
	 * However Vol 3, Part F, 3.3.1 says that multi-octet Characteristic
	 * Values shall be LE unless otherwise defined. PTS expects hash to be
	 * in little endianess as well. bt_smp_aes_cmac calculates the hash in
	 * big endianess so we have to swap.
	 */
	sys_mem_swap(db_hash, sizeof(db_hash));

	BT_HEXDUMP_DBG(db_hash, sizeof(db_hash), "Hash: ");

	if (IS_ENABLED(CONFIG_BT_SETTINGS) && store) {
		db_hash_store();
	}
}

static void db_hash_process(struct k_work *work)
{
	db_hash_gen(true);
}

static ssize_t db_hash_read(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr,
			    void *buf, uint16_t len, uint16_t offset)
{
	int err;

	/* Check if db_hash is already pending in which case it shall be
	 * generated immediately instead of waiting for the work to complete.
	 */
	err = k_delayed_work_cancel(&db_hash_work);
	if (!err) {
		db_hash_gen(true);
	}

	/* BLUETOOTH CORE SPECIFICATION Version 5.1 | Vol 3, Part G page 2347:
	 * 2.5.2.1 Robust Caching
	 * A connected client becomes change-aware when...
	 * The client reads the Database Hash characteristic and then the server
	 * receives another ATT request from the client.
	 */
	bt_gatt_change_aware(conn, true);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, db_hash,
				 sizeof(db_hash));
}

static void remove_cf_cfg(struct bt_conn *conn)
{
	struct gatt_cf_cfg *cfg;

	cfg = find_cf_cfg(conn);
	if (!cfg) {
		return;
	}

	/* BLUETOOTH CORE SPECIFICATION Version 5.1 | Vol 3, Part G page 2405:
	 * For clients with a trusted relationship, the characteristic value
	 * shall be persistent across connections. For clients without a
	 * trusted relationship the characteristic value shall be set to the
	 * default value at each connection.
	 */
	if (!bt_addr_le_is_bonded(conn->id, &conn->le.dst)) {
		clear_cf_cfg(cfg);
	} else {
		/* Update address in case it has changed */
		bt_addr_le_copy(&cfg->peer, &conn->le.dst);
		atomic_clear_bit(cfg->flags, CF_OUT_OF_SYNC);
	}
}

#if defined(CONFIG_BT_EATT)
#define SF_BIT_EATT	0
#define SF_BIT_LAST	SF_BIT_EATT

static ssize_t sf_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		       void *buf, uint16_t len, uint16_t offset)
{
	uint8_t value = BIT(SF_BIT_EATT);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &value,
				 sizeof(value));
}
#endif /* CONFIG_BT_EATT */
#endif /* CONFIG_BT_GATT_CACHING */

BT_GATT_SERVICE_DEFINE(_1_gatt_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_GATT),
#if defined(CONFIG_BT_GATT_SERVICE_CHANGED)
	/* Bluetooth 5.0, Vol3 Part G:
	 * The Service Changed characteristic Attribute Handle on the server
	 * shall not change if the server has a trusted relationship with any
	 * client.
	 */
	BT_GATT_CHARACTERISTIC(BT_UUID_GATT_SC, BT_GATT_CHRC_INDICATE,
			       BT_GATT_PERM_NONE, NULL, NULL, NULL),
	BT_GATT_CCC_MANAGED(&sc_ccc, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
#if defined(CONFIG_BT_GATT_CACHING)
	BT_GATT_CHARACTERISTIC(BT_UUID_GATT_CLIENT_FEATURES,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       cf_read, cf_write, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_GATT_DB_HASH,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       db_hash_read, NULL, NULL),
#if defined(CONFIG_BT_EATT)
	BT_GATT_CHARACTERISTIC(BT_UUID_GATT_SERVER_FEATURES,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       sf_read, NULL, NULL),
#endif /* CONFIG_BT_EATT */
#endif /* CONFIG_BT_GATT_CACHING */
#endif /* CONFIG_BT_GATT_SERVICE_CHANGED */
);

#if defined(CONFIG_BT_GATT_DYNAMIC_DB)
static uint8_t found_attr(const struct bt_gatt_attr *attr, uint16_t handle,
			  void *user_data)
{
	const struct bt_gatt_attr **found = user_data;

	*found = attr;

	return BT_GATT_ITER_STOP;
}

static const struct bt_gatt_attr *find_attr(uint16_t handle)
{
	const struct bt_gatt_attr *attr = NULL;

	bt_gatt_foreach_attr(handle, handle, found_attr, &attr);

	return attr;
}

static void gatt_insert(struct bt_gatt_service *svc, uint16_t last_handle)
{
	struct bt_gatt_service *tmp, *prev = NULL;

	if (last_handle == 0 || svc->attrs[0].handle > last_handle) {
		sys_slist_append(&db, &svc->node);
		return;
	}

	/* DB shall always have its service in ascending order */
	SYS_SLIST_FOR_EACH_CONTAINER(&db, tmp, node) {
		if (tmp->attrs[0].handle > svc->attrs[0].handle) {
			if (prev) {
				sys_slist_insert(&db, &prev->node, &svc->node);
			} else {
				sys_slist_prepend(&db, &svc->node);
			}
			return;
		}

		prev = tmp;
	}
}

static int gatt_register(struct bt_gatt_service *svc)
{
	struct bt_gatt_service *last;
	uint16_t handle, last_handle;
	struct bt_gatt_attr *attrs = svc->attrs;
	uint16_t count = svc->attr_count;

	if (sys_slist_is_empty(&db)) {
		handle = last_static_handle;
		last_handle = 0;
		goto populate;
	}

	last = SYS_SLIST_PEEK_TAIL_CONTAINER(&db, last, node);
	handle = last->attrs[last->attr_count - 1].handle;
	last_handle = handle;

populate:
	/* Populate the handles and append them to the list */
	for (; attrs && count; attrs++, count--) {
		if (!attrs->handle) {
			/* Allocate handle if not set already */
			attrs->handle = ++handle;
		} else if (attrs->handle > handle) {
			/* Use existing handle if valid */
			handle = attrs->handle;
		} else if (find_attr(attrs->handle)) {
			/* Service has conflicting handles */
			BT_ERR("Unable to register handle 0x%04x",
			       attrs->handle);
			return -EINVAL;
		}

		BT_DBG("attr %p handle 0x%04x uuid %s perm 0x%02x",
		       attrs, attrs->handle, bt_uuid_str(attrs->uuid),
		       attrs->perm);
	}

	gatt_insert(svc, last_handle);

	return 0;
}
#endif /* CONFIG_BT_GATT_DYNAMIC_DB */

enum {
	SC_RANGE_CHANGED,    /* SC range changed */
	SC_INDICATE_PENDING, /* SC indicate pending */

	/* Total number of flags - must be at the end of the enum */
	SC_NUM_FLAGS,
};

static struct gatt_sc {
	struct bt_gatt_indicate_params params;
	uint16_t start;
	uint16_t end;
	struct k_delayed_work work;
	ATOMIC_DEFINE(flags, SC_NUM_FLAGS);
} gatt_sc;

static void sc_indicate_rsp(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr, uint8_t err)
{
#if defined(CONFIG_BT_GATT_CACHING)
	struct gatt_cf_cfg *cfg;
#endif

	BT_DBG("err 0x%02x", err);

	atomic_clear_bit(gatt_sc.flags, SC_INDICATE_PENDING);

	/* Check if there is new change in the meantime */
	if (atomic_test_bit(gatt_sc.flags, SC_RANGE_CHANGED)) {
		/* Reschedule without any delay since it is waiting already */
		k_delayed_work_submit(&gatt_sc.work, K_NO_WAIT);
	}

#if defined(CONFIG_BT_GATT_CACHING)
	/* BLUETOOTH CORE SPECIFICATION Version 5.1 | Vol 3, Part G page 2347:
	 * 2.5.2.1 Robust Caching
	 * A connected client becomes change-aware when...
	 * The client receives and confirms a Service Changed indication.
	 */
	cfg = find_cf_cfg(conn);
	if (cfg && CF_ROBUST_CACHING(cfg)) {
		atomic_set_bit(cfg->flags, CF_CHANGE_AWARE);
		BT_DBG("%s change-aware", bt_addr_le_str(&cfg->peer));
	}
#endif
}

static void sc_process(struct k_work *work)
{
	struct gatt_sc *sc = CONTAINER_OF(work, struct gatt_sc, work);
	uint16_t sc_range[2];

	__ASSERT(!atomic_test_bit(sc->flags, SC_INDICATE_PENDING),
		 "Indicate already pending");

	BT_DBG("start 0x%04x end 0x%04x", sc->start, sc->end);

	sc_range[0] = sys_cpu_to_le16(sc->start);
	sc_range[1] = sys_cpu_to_le16(sc->end);

	atomic_clear_bit(sc->flags, SC_RANGE_CHANGED);
	sc->start = 0U;
	sc->end = 0U;

	sc->params.attr = &_1_gatt_svc.attrs[2];
	sc->params.func = sc_indicate_rsp;
	sc->params.data = &sc_range[0];
	sc->params.len = sizeof(sc_range);

	if (bt_gatt_indicate(NULL, &sc->params)) {
		/* No connections to indicate */
		return;
	}

	atomic_set_bit(sc->flags, SC_INDICATE_PENDING);
}

static void clear_ccc_cfg(struct bt_gatt_ccc_cfg *cfg)
{
	bt_addr_le_copy(&cfg->peer, BT_ADDR_LE_ANY);
	cfg->id = 0U;
	cfg->value = 0U;
}

#if defined(CONFIG_BT_SETTINGS_CCC_STORE_ON_WRITE)
static struct gatt_ccc_store {
	struct bt_conn *conn_list[CONFIG_BT_MAX_CONN];
	struct k_delayed_work work;
} gatt_ccc_store;

static bool gatt_ccc_conn_is_queued(struct bt_conn *conn)
{
	return (conn == gatt_ccc_store.conn_list[bt_conn_index(conn)]);
}

static void gatt_ccc_conn_unqueue(struct bt_conn *conn)
{
	uint8_t index = bt_conn_index(conn);

	if (gatt_ccc_store.conn_list[index] != NULL) {
		bt_conn_unref(gatt_ccc_store.conn_list[index]);
		gatt_ccc_store.conn_list[index] = NULL;
	}
}

static void gatt_ccc_conn_enqueue(struct bt_conn *conn)
{
	if ((!gatt_ccc_conn_is_queued(conn)) &&
	    bt_addr_le_is_bonded(conn->id, &conn->le.dst)) {
		/* Store the connection with the same index it has in
		 * the conns array
		 */
		gatt_ccc_store.conn_list[bt_conn_index(conn)] =
			bt_conn_ref(conn);

		k_delayed_work_submit(&gatt_ccc_store.work, CCC_STORE_DELAY);
	}
}

static bool gatt_ccc_conn_queue_is_empty(void)
{
	for (size_t i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if (gatt_ccc_store.conn_list[i]) {
			return false;
		}
	}

	return true;
}

static void ccc_delayed_store(struct k_work *work)
{
	struct gatt_ccc_store *ccc_store =
		CONTAINER_OF(work, struct gatt_ccc_store, work);

	for (size_t i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		struct bt_conn *conn = ccc_store->conn_list[i];

		if (!conn) {
			continue;
		}

		if (bt_addr_le_is_bonded(conn->id, &conn->le.dst)) {
			ccc_store->conn_list[i] = NULL;
			bt_gatt_store_ccc(conn->id, &conn->le.dst);
			bt_conn_unref(conn);
		}
	}
}
#endif

void bt_gatt_init(void)
{
	if (!atomic_cas(&init, 0, 1)) {
		return;
	}

	Z_STRUCT_SECTION_FOREACH(bt_gatt_service_static, svc) {
		last_static_handle += svc->attr_count;
	}

#if defined(CONFIG_BT_GATT_CACHING)
	k_delayed_work_init(&db_hash_work, db_hash_process);

	/* Submit work to Generate initial hash as there could be static
	 * services already in the database.
	 */
	k_delayed_work_submit(&db_hash_work, DB_HASH_TIMEOUT);
#endif /* CONFIG_BT_GATT_CACHING */

	if (IS_ENABLED(CONFIG_BT_GATT_SERVICE_CHANGED)) {
		k_delayed_work_init(&gatt_sc.work, sc_process);
		if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
			/* Make sure to not send SC indications until SC
			 * settings are loaded
			 */
			atomic_set_bit(gatt_sc.flags, SC_INDICATE_PENDING);
		}
	}

#if defined(CONFIG_BT_SETTINGS_CCC_STORE_ON_WRITE)
	k_delayed_work_init(&gatt_ccc_store.work, ccc_delayed_store);
#endif
}

#if defined(CONFIG_BT_GATT_DYNAMIC_DB) || \
    (defined(CONFIG_BT_GATT_CACHING) && defined(CONFIG_BT_SETTINGS))
static void sc_indicate(uint16_t start, uint16_t end)
{
	BT_DBG("start 0x%04x end 0x%04x", start, end);

	if (!atomic_test_and_set_bit(gatt_sc.flags, SC_RANGE_CHANGED)) {
		gatt_sc.start = start;
		gatt_sc.end = end;
		goto submit;
	}

	if (!update_range(&gatt_sc.start, &gatt_sc.end, start, end)) {
		return;
	}

submit:
	if (atomic_test_bit(gatt_sc.flags, SC_INDICATE_PENDING)) {
		BT_DBG("indicate pending, waiting until complete...");
		return;
	}

	/* Reschedule since the range has changed */
	k_delayed_work_submit(&gatt_sc.work, SC_TIMEOUT);
}
#endif /* BT_GATT_DYNAMIC_DB || (BT_GATT_CACHING && BT_SETTINGS) */

#if defined(CONFIG_BT_GATT_DYNAMIC_DB)
static void db_changed(void)
{
#if defined(CONFIG_BT_GATT_CACHING)
	int i;

	k_delayed_work_submit(&db_hash_work, DB_HASH_TIMEOUT);

	for (i = 0; i < ARRAY_SIZE(cf_cfg); i++) {
		struct gatt_cf_cfg *cfg = &cf_cfg[i];

		if (!bt_addr_le_cmp(&cfg->peer, BT_ADDR_LE_ANY)) {
			continue;
		}

		if (CF_ROBUST_CACHING(cfg)) {
			/* Core Spec 5.1 | Vol 3, Part G, 2.5.2.1 Robust Caching
			 *... the database changes again before the client
			 * becomes change-aware in which case the error response
			 * shall be sent again.
			 */
			atomic_clear_bit(cfg->flags, CF_OUT_OF_SYNC);
			if (atomic_test_and_clear_bit(cfg->flags,
						      CF_CHANGE_AWARE)) {
				BT_DBG("%s change-unaware",
				       bt_addr_le_str(&cfg->peer));
			}
		}
	}
#endif
}

static void gatt_unregister_ccc(struct _bt_gatt_ccc *ccc)
{
	ccc->value = 0;

	for (size_t i = 0; i < ARRAY_SIZE(ccc->cfg); i++) {
		struct bt_gatt_ccc_cfg *cfg = &ccc->cfg[i];

		if (bt_addr_le_cmp(&cfg->peer, BT_ADDR_LE_ANY)) {
			struct bt_conn *conn;
			bool store = true;

			conn = bt_conn_lookup_addr_le(cfg->id, &cfg->peer);
			if (conn) {
				if (conn->state == BT_CONN_CONNECTED) {
#if defined(CONFIG_BT_SETTINGS_CCC_STORE_ON_WRITE)
					gatt_ccc_conn_enqueue(conn);
#endif
					store = false;
				}

				bt_conn_unref(conn);
			}

			if (IS_ENABLED(CONFIG_BT_SETTINGS) && store &&
			    bt_addr_le_is_bonded(cfg->id, &cfg->peer)) {
				bt_gatt_store_ccc(cfg->id, &cfg->peer);
			}

			clear_ccc_cfg(cfg);
		}
	}
}

static int gatt_unregister(struct bt_gatt_service *svc)
{
	if (!sys_slist_find_and_remove(&db, &svc->node)) {
		return -ENOENT;
	}

	for (uint16_t i = 0; i < svc->attr_count; i++) {
		struct bt_gatt_attr *attr = &svc->attrs[i];

		if (attr->write == bt_gatt_attr_write_ccc) {
			gatt_unregister_ccc(attr->user_data);
		}
	}

	return 0;
}

int bt_gatt_service_register(struct bt_gatt_service *svc)
{
	int err;

	__ASSERT(svc, "invalid parameters\n");
	__ASSERT(svc->attrs, "invalid parameters\n");
	__ASSERT(svc->attr_count, "invalid parameters\n");

	/* Init GATT core services */
	bt_gatt_init();

	/* Do no allow to register mandatory services twice */
	if (!bt_uuid_cmp(svc->attrs[0].uuid, BT_UUID_GAP) ||
	    !bt_uuid_cmp(svc->attrs[0].uuid, BT_UUID_GATT)) {
		return -EALREADY;
	}

	err = gatt_register(svc);
	if (err < 0) {
		return err;
	}

	sc_indicate(svc->attrs[0].handle,
		    svc->attrs[svc->attr_count - 1].handle);

	db_changed();

	return 0;
}

int bt_gatt_service_unregister(struct bt_gatt_service *svc)
{
	int err;

	__ASSERT(svc, "invalid parameters\n");

	err = gatt_unregister(svc);
	if (err) {
		return err;
	}

	sc_indicate(svc->attrs[0].handle,
		    svc->attrs[svc->attr_count - 1].handle);

	db_changed();

	return 0;
}
#endif /* CONFIG_BT_GATT_DYNAMIC_DB */

ssize_t bt_gatt_attr_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			  void *buf, uint16_t buf_len, uint16_t offset,
			  const void *value, uint16_t value_len)
{
	uint16_t len;

	if (offset > value_len) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	len = MIN(buf_len, value_len - offset);

	BT_DBG("handle 0x%04x offset %u length %u", attr->handle, offset,
	       len);

	memcpy(buf, (uint8_t *)value + offset, len);

	return len;
}

ssize_t bt_gatt_attr_read_service(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  void *buf, uint16_t len, uint16_t offset)
{
	struct bt_uuid *uuid = attr->user_data;

	if (uuid->type == BT_UUID_TYPE_16) {
		uint16_t uuid16 = sys_cpu_to_le16(BT_UUID_16(uuid)->val);

		return bt_gatt_attr_read(conn, attr, buf, len, offset,
					 &uuid16, 2);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 BT_UUID_128(uuid)->val, 16);
}

struct gatt_incl {
	uint16_t start_handle;
	uint16_t end_handle;
	uint16_t uuid16;
} __packed;

static uint8_t get_service_handles(const struct bt_gatt_attr *attr,
				   uint16_t handle, void *user_data)
{
	struct gatt_incl *include = user_data;

	/* Stop if attribute is a service */
	if (!bt_uuid_cmp(attr->uuid, BT_UUID_GATT_PRIMARY) ||
	    !bt_uuid_cmp(attr->uuid, BT_UUID_GATT_SECONDARY)) {
		return BT_GATT_ITER_STOP;
	}

	include->end_handle = handle;

	return BT_GATT_ITER_CONTINUE;
}

uint16_t bt_gatt_attr_get_handle(const struct bt_gatt_attr *attr)
{
	uint16_t handle = 1;

	if (!attr) {
		return 0;
	}

	if (attr->handle) {
		return attr->handle;
	}

	Z_STRUCT_SECTION_FOREACH(bt_gatt_service_static, static_svc) {
		/* Skip ahead if start is not within service attributes array */
		if ((attr < &static_svc->attrs[0]) ||
		    (attr > &static_svc->attrs[static_svc->attr_count - 1])) {
			handle += static_svc->attr_count;
			continue;
		}

		for (size_t i = 0; i < static_svc->attr_count; i++, handle++) {
			if (attr == &static_svc->attrs[i]) {
				return handle;
			}
		}
	}

	return 0;
}

ssize_t bt_gatt_attr_read_included(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr,
				   void *buf, uint16_t len, uint16_t offset)
{
	struct bt_gatt_attr *incl = attr->user_data;
	uint16_t handle = bt_gatt_attr_get_handle(incl);
	struct bt_uuid *uuid = incl->user_data;
	struct gatt_incl pdu;
	uint8_t value_len;

	/* first attr points to the start handle */
	pdu.start_handle = sys_cpu_to_le16(handle);
	value_len = sizeof(pdu.start_handle) + sizeof(pdu.end_handle);

	/*
	 * Core 4.2, Vol 3, Part G, 3.2,
	 * The Service UUID shall only be present when the UUID is a
	 * 16-bit Bluetooth UUID.
	 */
	if (uuid->type == BT_UUID_TYPE_16) {
		pdu.uuid16 = sys_cpu_to_le16(BT_UUID_16(uuid)->val);
		value_len += sizeof(pdu.uuid16);
	}

	/* Lookup for service end handle */
	bt_gatt_foreach_attr(handle + 1, 0xffff, get_service_handles, &pdu);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &pdu, value_len);
}

struct gatt_chrc {
	uint8_t properties;
	uint16_t value_handle;
	union {
		uint16_t uuid16;
		uint8_t  uuid[16];
	};
} __packed;

uint16_t bt_gatt_attr_value_handle(const struct bt_gatt_attr *attr)
{
	uint16_t handle = 0;

	if ((attr != NULL)
	    && (attr->read == bt_gatt_attr_read_chrc)) {
		struct bt_gatt_chrc *chrc = attr->user_data;

		handle = chrc->value_handle;
		if (handle == 0) {
			/* Fall back to Zephyr value handle policy */
			handle = bt_gatt_attr_get_handle(attr) + 1U;
		}
	}

	return handle;
}

ssize_t bt_gatt_attr_read_chrc(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	struct bt_gatt_chrc *chrc = attr->user_data;
	struct gatt_chrc pdu;
	uint8_t value_len;

	pdu.properties = chrc->properties;
	/* BLUETOOTH SPECIFICATION Version 4.2 [Vol 3, Part G] page 534:
	 * 3.3.2 Characteristic Value Declaration
	 * The Characteristic Value declaration contains the value of the
	 * characteristic. It is the first Attribute after the characteristic
	 * declaration. All characteristic definitions shall have a
	 * Characteristic Value declaration.
	 */
	pdu.value_handle = sys_cpu_to_le16(bt_gatt_attr_value_handle(attr));

	value_len = sizeof(pdu.properties) + sizeof(pdu.value_handle);

	if (chrc->uuid->type == BT_UUID_TYPE_16) {
		pdu.uuid16 = sys_cpu_to_le16(BT_UUID_16(chrc->uuid)->val);
		value_len += 2U;
	} else {
		memcpy(pdu.uuid, BT_UUID_128(chrc->uuid)->val, 16);
		value_len += 16U;
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &pdu, value_len);
}

static uint8_t gatt_foreach_iter(const struct bt_gatt_attr *attr,
				 uint16_t handle, uint16_t start_handle,
				 uint16_t end_handle,
				 const struct bt_uuid *uuid,
				 const void *attr_data, uint16_t *num_matches,
				 bt_gatt_attr_func_t func, void *user_data)
{
	uint8_t result;

	/* Stop if over the requested range */
	if (handle > end_handle) {
		return BT_GATT_ITER_STOP;
	}

	/* Check if attribute handle is within range */
	if (handle < start_handle) {
		return BT_GATT_ITER_CONTINUE;
	}

	/* Match attribute UUID if set */
	if (uuid && bt_uuid_cmp(uuid, attr->uuid)) {
		return BT_GATT_ITER_CONTINUE;
	}

	/* Match attribute user_data if set */
	if (attr_data && attr_data != attr->user_data) {
		return BT_GATT_ITER_CONTINUE;
	}

	*num_matches -= 1;

	result = func(attr, handle, user_data);

	if (!*num_matches) {
		return BT_GATT_ITER_STOP;
	}

	return result;
}

static void foreach_attr_type_dyndb(uint16_t start_handle, uint16_t end_handle,
				    const struct bt_uuid *uuid,
				    const void *attr_data, uint16_t num_matches,
				    bt_gatt_attr_func_t func, void *user_data)
{
#if defined(CONFIG_BT_GATT_DYNAMIC_DB)
	size_t i;
	struct bt_gatt_service *svc;

	SYS_SLIST_FOR_EACH_CONTAINER(&db, svc, node) {
		struct bt_gatt_service *next;

		next = SYS_SLIST_PEEK_NEXT_CONTAINER(svc, node);
		if (next) {
			/* Skip ahead if start is not within service handles */
			if (next->attrs[0].handle <= start_handle) {
				continue;
			}
		}

		for (i = 0; i < svc->attr_count; i++) {
			struct bt_gatt_attr *attr = &svc->attrs[i];

			if (gatt_foreach_iter(attr, attr->handle,
					      start_handle,
					      end_handle,
					      uuid, attr_data,
					      &num_matches,
					      func, user_data) ==
			    BT_GATT_ITER_STOP) {
				return;
			}
		}
	}
#endif /* CONFIG_BT_GATT_DYNAMIC_DB */
}

void bt_gatt_foreach_attr_type(uint16_t start_handle, uint16_t end_handle,
			       const struct bt_uuid *uuid,
			       const void *attr_data, uint16_t num_matches,
			       bt_gatt_attr_func_t func, void *user_data)
{
	size_t i;

	if (!num_matches) {
		num_matches = UINT16_MAX;
	}

	if (start_handle <= last_static_handle) {
		uint16_t handle = 1;

		Z_STRUCT_SECTION_FOREACH(bt_gatt_service_static, static_svc) {
			/* Skip ahead if start is not within service handles */
			if (handle + static_svc->attr_count < start_handle) {
				handle += static_svc->attr_count;
				continue;
			}

			for (i = 0; i < static_svc->attr_count; i++, handle++) {
				if (gatt_foreach_iter(&static_svc->attrs[i],
						      handle, start_handle,
						      end_handle, uuid,
						      attr_data, &num_matches,
						      func, user_data) ==
				    BT_GATT_ITER_STOP) {
					return;
				}
			}
		}
	}

	/* Iterate over dynamic db */
	foreach_attr_type_dyndb(start_handle, end_handle, uuid, attr_data,
				num_matches, func, user_data);
}

static uint8_t find_next(const struct bt_gatt_attr *attr, uint16_t handle,
			 void *user_data)
{
	struct bt_gatt_attr **next = user_data;

	*next = (struct bt_gatt_attr *)attr;

	return BT_GATT_ITER_STOP;
}

struct bt_gatt_attr *bt_gatt_attr_next(const struct bt_gatt_attr *attr)
{
	struct bt_gatt_attr *next = NULL;
	uint16_t handle = bt_gatt_attr_get_handle(attr);

	bt_gatt_foreach_attr(handle + 1, handle + 1, find_next, &next);

	return next;
}

static struct bt_gatt_ccc_cfg *find_ccc_cfg(const struct bt_conn *conn,
					    struct _bt_gatt_ccc *ccc)
{
	for (size_t i = 0; i < ARRAY_SIZE(ccc->cfg); i++) {
		struct bt_gatt_ccc_cfg *cfg = &ccc->cfg[i];

		if (conn) {
			if (bt_conn_is_peer_addr_le(conn, cfg->id,
						    &cfg->peer)) {
				return cfg;
			}
		} else if (!bt_addr_le_cmp(&cfg->peer, BT_ADDR_LE_ANY)) {
			return cfg;
		}
	}

	return NULL;
}

ssize_t bt_gatt_attr_read_ccc(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, void *buf,
			      uint16_t len, uint16_t offset)
{
	struct _bt_gatt_ccc *ccc = attr->user_data;
	const struct bt_gatt_ccc_cfg *cfg;
	uint16_t value;

	cfg = find_ccc_cfg(conn, ccc);
	if (cfg) {
		value = sys_cpu_to_le16(cfg->value);
	} else {
		/* Default to disable if there is no cfg for the peer */
		value = 0x0000;
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &value,
				 sizeof(value));
}

static void gatt_ccc_changed(const struct bt_gatt_attr *attr,
			     struct _bt_gatt_ccc *ccc)
{
	int i;
	uint16_t value = 0x0000;

	for (i = 0; i < ARRAY_SIZE(ccc->cfg); i++) {
		if (ccc->cfg[i].value > value) {
			value = ccc->cfg[i].value;
		}
	}

	BT_DBG("ccc %p value 0x%04x", ccc, value);

	if (value != ccc->value) {
		ccc->value = value;
		if (ccc->cfg_changed) {
			ccc->cfg_changed(attr, value);
		}
	}
}

ssize_t bt_gatt_attr_write_ccc(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, const void *buf,
			       uint16_t len, uint16_t offset, uint8_t flags)
{
	struct _bt_gatt_ccc *ccc = attr->user_data;
	struct bt_gatt_ccc_cfg *cfg;
	uint16_t value;

	if (offset) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (!len || len > sizeof(uint16_t)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (len < sizeof(uint16_t)) {
		value = *(uint8_t *)buf;
	} else {
		value = sys_get_le16(buf);
	}

	cfg = find_ccc_cfg(conn, ccc);
	if (!cfg) {
		/* If there's no existing entry, but the new value is zero,
		 * we don't need to do anything, since a disabled CCC is
		 * behavioraly the same as no written CCC.
		 */
		if (!value) {
			return len;
		}

		cfg = find_ccc_cfg(NULL, ccc);
		if (!cfg) {
			BT_WARN("No space to store CCC cfg");
			return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
		}

		bt_addr_le_copy(&cfg->peer, &conn->le.dst);
		cfg->id = conn->id;
	}

	/* Confirm write if cfg is managed by application */
	if (ccc->cfg_write) {
		ssize_t write = ccc->cfg_write(conn, attr, value);

		if (write < 0) {
			return write;
		}

		/* Accept size=1 for backwards compatibility */
		if (write != sizeof(value) && write != 1) {
			return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		}
	}

	cfg->value = value;

	BT_DBG("handle 0x%04x value %u", attr->handle, cfg->value);

	/* Update cfg if don't match */
	if (cfg->value != ccc->value) {
		gatt_ccc_changed(attr, ccc);

#if defined(CONFIG_BT_SETTINGS_CCC_STORE_ON_WRITE)
		gatt_ccc_conn_enqueue(conn);
#endif
	}

	/* Disabled CCC is the same as no configured CCC, so clear the entry */
	if (!value) {
		clear_ccc_cfg(cfg);
	}

	return len;
}

ssize_t bt_gatt_attr_read_cep(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, void *buf,
			      uint16_t len, uint16_t offset)
{
	const struct bt_gatt_cep *value = attr->user_data;
	uint16_t props = sys_cpu_to_le16(value->properties);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &props,
				 sizeof(props));
}

ssize_t bt_gatt_attr_read_cud(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, void *buf,
			      uint16_t len, uint16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 strlen(value));
}

ssize_t bt_gatt_attr_read_cpf(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, void *buf,
			      uint16_t len, uint16_t offset)
{
	const struct bt_gatt_cpf *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(*value));
}

struct notify_data {
	int err;
	uint16_t type;
	union {
		struct bt_gatt_notify_params *nfy_params;
		struct bt_gatt_indicate_params *ind_params;
	};
};

#if defined(CONFIG_BT_GATT_NOTIFY_MULTIPLE)

struct nfy_mult_data {
	bt_gatt_complete_func_t func;
	void *user_data;
};

#define nfy_mult_user_data(buf) \
	((struct nfy_mult_data *)net_buf_user_data(buf))
#define nfy_mult_data_match(buf, _func, _user_data) \
	((nfy_mult_user_data(buf)->func == _func) && \
	(nfy_mult_user_data(buf)->user_data == _user_data))

static struct net_buf *nfy_mult[CONFIG_BT_MAX_CONN];

static int gatt_notify_mult_send(struct bt_conn *conn, struct net_buf **buf)
{
	struct nfy_mult_data *data = nfy_mult_user_data(*buf);
	int ret;

	ret = bt_att_send(conn, *buf, data->func, data->user_data);
	if (ret < 0) {
		net_buf_unref(*buf);
	}

	*buf = NULL;

	return ret;
}

static void notify_mult_process(struct k_work *work)
{
	int i;

	/* Send to any connection with an allocated buffer */
	for (i = 0; i < ARRAY_SIZE(nfy_mult); i++) {
		struct net_buf **buf = &nfy_mult[i];

		if (*buf) {
			struct bt_conn *conn = bt_conn_lookup_index(i);

			gatt_notify_mult_send(conn, buf);
			bt_conn_unref(conn);
		}
	}
}

K_WORK_DEFINE(nfy_mult_work, notify_mult_process);

static bool gatt_cf_notify_multi(struct bt_conn *conn)
{
	struct gatt_cf_cfg *cfg;

	cfg = find_cf_cfg(conn);
	if (!cfg) {
		return false;
	}

	return CF_NOTIFY_MULTI(cfg);
}

static int gatt_notify_mult(struct bt_conn *conn, uint16_t handle,
			    struct bt_gatt_notify_params *params)
{
	struct net_buf **buf = &nfy_mult[bt_conn_index(conn)];
	struct bt_att_notify_mult *nfy;

	/* Check if we can fit more data into it, in case it doesn't fit send
	 * the existing buffer and proceed to create a new one
	 */
	if (*buf && ((net_buf_tailroom(*buf) < sizeof(*nfy) + params->len) ||
	    !nfy_mult_data_match(*buf, params->func, params->user_data))) {
		int ret;

		ret = gatt_notify_mult_send(conn, buf);
		if (ret < 0) {
			return ret;
		}
	}

	if (!*buf) {
		*buf = bt_att_create_pdu(conn, BT_ATT_OP_NOTIFY_MULT,
					 sizeof(*nfy) + params->len);
		if (!*buf) {
			return -ENOMEM;
		}
		/* Set user_data so it can be restored when sending */
		nfy_mult_user_data(*buf)->func = params->func;
		nfy_mult_user_data(*buf)->user_data = params->user_data;
	}

	BT_DBG("handle 0x%04x len %u", handle, params->len);

	nfy = net_buf_add(*buf, sizeof(*nfy));
	nfy->handle = sys_cpu_to_le16(handle);
	nfy->len = sys_cpu_to_le16(params->len);

	net_buf_add(*buf, params->len);
	memcpy(nfy->value, params->data, params->len);

	k_work_submit(&nfy_mult_work);

	return 0;
}
#endif /* CONFIG_BT_GATT_NOTIFY_MULTIPLE */

static int gatt_notify(struct bt_conn *conn, uint16_t handle,
		       struct bt_gatt_notify_params *params)
{
	struct net_buf *buf;
	struct bt_att_notify *nfy;

#if defined(CONFIG_BT_GATT_ENFORCE_CHANGE_UNAWARE)
	/* BLUETOOTH CORE SPECIFICATION Version 5.1 | Vol 3, Part G page 2350:
	 * Except for the Handle Value indication, the  server shall not send
	 * notifications and indications to such a client until it becomes
	 * change-aware.
	 */
	if (!bt_gatt_change_aware(conn, false)) {
		return -EAGAIN;
	}
#endif

#if defined(CONFIG_BT_GATT_NOTIFY_MULTIPLE)
	if (gatt_cf_notify_multi(conn)) {
		int err;

		err = gatt_notify_mult(conn, handle, params);
		if (err && err != -ENOMEM) {
			return err;
		}
	}
#endif /* CONFIG_BT_GATT_NOTIFY_MULTIPLE */

	buf = bt_att_create_pdu(conn, BT_ATT_OP_NOTIFY,
				sizeof(*nfy) + params->len);
	if (!buf) {
		BT_WARN("No buffer available to send notification");
		return -ENOMEM;
	}

	BT_DBG("conn %p handle 0x%04x", conn, handle);

	nfy = net_buf_add(buf, sizeof(*nfy));
	nfy->handle = sys_cpu_to_le16(handle);

	net_buf_add(buf, params->len);
	memcpy(nfy->value, params->data, params->len);

	return bt_att_send(conn, buf, params->func, params->user_data);
}

static void gatt_indicate_rsp(struct bt_conn *conn, uint8_t err,
			      const void *pdu, uint16_t length, void *user_data)
{
	struct bt_gatt_indicate_params *params = user_data;

	params->func(conn, params->attr, err);
}

static int gatt_send(struct bt_conn *conn, struct net_buf *buf,
		     bt_att_func_t func, void *params,
		     bt_att_destroy_t destroy)
{
	int err;

	if (params) {
		struct bt_att_req *req;

		/* Allocate new request */
		req = bt_att_req_alloc(BT_ATT_TIMEOUT);
		if (!req) {
			return -ENOMEM;
		}

		req->buf = buf;
		req->func = func;
		req->destroy = destroy;
		req->user_data = params;

		err = bt_att_req_send(conn, req);
		if (err) {
			bt_att_req_free(req);
		}
	} else {
		err = bt_att_send(conn, buf, NULL, NULL);
	}

	if (err) {
		BT_ERR("Error sending ATT PDU: %d", err);
	}

	return err;
}

static int gatt_indicate(struct bt_conn *conn, uint16_t handle,
			 struct bt_gatt_indicate_params *params)
{
	struct net_buf *buf;
	struct bt_att_indicate *ind;

#if defined(CONFIG_BT_GATT_ENFORCE_CHANGE_UNAWARE)
	/* BLUETOOTH CORE SPECIFICATION Version 5.1 | Vol 3, Part G page 2350:
	 * Except for the Handle Value indication, the  server shall not send
	 * notifications and indications to such a client until it becomes
	 * change-aware.
	 */
	if (!(params->func && (params->func == sc_indicate_rsp ||
	    params->func == sc_restore_rsp)) &&
	    !bt_gatt_change_aware(conn, false)) {
		return -EAGAIN;
	}
#endif

	buf = bt_att_create_pdu(conn, BT_ATT_OP_INDICATE,
				sizeof(*ind) + params->len);
	if (!buf) {
		BT_WARN("No buffer available to send indication");
		return -ENOMEM;
	}

	BT_DBG("conn %p handle 0x%04x", conn, handle);

	ind = net_buf_add(buf, sizeof(*ind));
	ind->handle = sys_cpu_to_le16(handle);

	net_buf_add(buf, params->len);
	memcpy(ind->value, params->data, params->len);

	if (!params->func) {
		return gatt_send(conn, buf, NULL, NULL, NULL);
	}

	return gatt_send(conn, buf, gatt_indicate_rsp, params, NULL);
}

static uint8_t notify_cb(const struct bt_gatt_attr *attr, uint16_t handle,
			 void *user_data)
{
	struct notify_data *data = user_data;
	struct _bt_gatt_ccc *ccc;
	size_t i;

	/* Check attribute user_data must be of type struct _bt_gatt_ccc */
	if (attr->write != bt_gatt_attr_write_ccc) {
		return BT_GATT_ITER_CONTINUE;
	}

	ccc = attr->user_data;

	/* Save Service Changed data if peer is not connected */
	if (IS_ENABLED(CONFIG_BT_GATT_SERVICE_CHANGED) && ccc == &sc_ccc) {
		for (i = 0; i < ARRAY_SIZE(sc_cfg); i++) {
			struct gatt_sc_cfg *cfg = &sc_cfg[i];
			struct bt_conn *conn;

			if (!bt_addr_le_cmp(&cfg->peer, BT_ADDR_LE_ANY)) {
				continue;
			}

			conn = bt_conn_lookup_state_le(cfg->id, &cfg->peer,
						       BT_CONN_CONNECTED);
			if (!conn) {
				struct sc_data *sc;

				sc = (struct sc_data *)data->ind_params->data;
				sc_save(cfg->id, &cfg->peer,
					sys_le16_to_cpu(sc->start),
					sys_le16_to_cpu(sc->end));
				continue;
			}

			bt_conn_unref(conn);
		}
	}

	/* Notify all peers configured */
	for (i = 0; i < ARRAY_SIZE(ccc->cfg); i++) {
		struct bt_gatt_ccc_cfg *cfg = &ccc->cfg[i];
		struct bt_conn *conn;
		int err;

		/* Check if config value matches data type since consolidated
		 * value may be for a different peer.
		 */
		if (cfg->value != data->type) {
			continue;
		}

		conn = bt_conn_lookup_addr_le(cfg->id, &cfg->peer);
		if (!conn) {
			continue;
		}

		if (conn->state != BT_CONN_CONNECTED) {
			bt_conn_unref(conn);
			continue;
		}

		/* Confirm match if cfg is managed by application */
		if (ccc->cfg_match && !ccc->cfg_match(conn, attr)) {
			bt_conn_unref(conn);
			continue;
		}

		if (data->type == BT_GATT_CCC_INDICATE) {
			err = gatt_indicate(conn, handle - 1, data->ind_params);
		} else {
			err = gatt_notify(conn, handle - 1, data->nfy_params);
		}

		bt_conn_unref(conn);

		if (err < 0) {
			return BT_GATT_ITER_STOP;
		}

		data->err = 0;
	}

	return BT_GATT_ITER_CONTINUE;
}

struct find_data {
	const struct bt_gatt_attr *attr;
	uint16_t handle;
};

static uint8_t match_uuid(const struct bt_gatt_attr *attr, uint16_t handle,
			  void *user_data)
{
	struct find_data *found = user_data;

	found->attr = attr;
	found->handle = handle;

	return BT_GATT_ITER_STOP;
}

static bool gatt_find_by_uuid(struct find_data *found,
			      const struct bt_uuid *uuid)
{
	found->attr = NULL;

	bt_gatt_foreach_attr_type(found->handle, 0xffff, uuid, NULL, 1,
				  match_uuid, found);

	return found->attr ? true : false;
}

int bt_gatt_notify_cb(struct bt_conn *conn,
		      struct bt_gatt_notify_params *params)
{
	struct notify_data data;
	struct find_data found;

	__ASSERT(params, "invalid parameters\n");
	__ASSERT(params->attr, "invalid parameters\n");

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		return -EAGAIN;
	}

	found.attr = params->attr;

	if (conn && conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	found.handle = bt_gatt_attr_get_handle(found.attr);
	if (!found.handle) {
		return -ENOENT;
	}

	/* Lookup UUID if it was given */
	if (params->uuid) {
		if (!gatt_find_by_uuid(&found, params->uuid)) {
			return -ENOENT;
		}
	}

	/* Check if attribute is a characteristic then adjust the handle */
	if (!bt_uuid_cmp(found.attr->uuid, BT_UUID_GATT_CHRC)) {
		struct bt_gatt_chrc *chrc = found.attr->user_data;

		if (!(chrc->properties & BT_GATT_CHRC_NOTIFY)) {
			return -EINVAL;
		}

		found.handle = bt_gatt_attr_value_handle(found.attr);
	}

	if (conn) {
		return gatt_notify(conn, found.handle, params);
	}

	data.err = -ENOTCONN;
	data.type = BT_GATT_CCC_NOTIFY;
	data.nfy_params = params;

	bt_gatt_foreach_attr_type(found.handle, 0xffff, BT_UUID_GATT_CCC, NULL,
				  1, notify_cb, &data);

	return data.err;
}

#if defined(CONFIG_BT_GATT_NOTIFY_MULTIPLE)
int bt_gatt_notify_multiple(struct bt_conn *conn, uint16_t num_params,
			    struct bt_gatt_notify_params *params)
{
	int i, ret;

	__ASSERT(params, "invalid parameters\n");
	__ASSERT(num_params, "invalid parameters\n");
	__ASSERT(params->attr, "invalid parameters\n");

	for (i = 0; i < num_params; i++) {
		ret = bt_gatt_notify_cb(conn, &params[i]);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}
#endif /* CONFIG_BT_GATT_NOTIFY_MULTIPLE */

int bt_gatt_indicate(struct bt_conn *conn,
		     struct bt_gatt_indicate_params *params)
{
	struct notify_data data;
	struct find_data found;

	__ASSERT(params, "invalid parameters\n");
	__ASSERT(params->attr, "invalid parameters\n");

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		return -EAGAIN;
	}

	found.attr = params->attr;

	if (conn && conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	found.handle = bt_gatt_attr_get_handle(found.attr);
	if (!found.handle) {
		return -ENOENT;
	}

	/* Lookup UUID if it was given */
	if (params->uuid) {
		if (!gatt_find_by_uuid(&found, params->uuid)) {
			return -ENOENT;
		}
	}

	/* Check if attribute is a characteristic then adjust the handle */
	if (!bt_uuid_cmp(found.attr->uuid, BT_UUID_GATT_CHRC)) {
		struct bt_gatt_chrc *chrc = found.attr->user_data;

		if (!(chrc->properties & BT_GATT_CHRC_INDICATE)) {
			return -EINVAL;
		}

		found.handle = bt_gatt_attr_value_handle(found.attr);
	}

	if (conn) {
		return gatt_indicate(conn, found.handle, params);
	}

	data.err = -ENOTCONN;
	data.type = BT_GATT_CCC_INDICATE;
	data.ind_params = params;

	bt_gatt_foreach_attr_type(found.handle, 0xffff, BT_UUID_GATT_CCC, NULL,
				  1, notify_cb, &data);

	return data.err;
}

uint16_t bt_gatt_get_mtu(struct bt_conn *conn)
{
	return bt_att_get_mtu(conn);
}

uint8_t bt_gatt_check_perm(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			uint8_t mask)
{
	if ((mask & BT_GATT_PERM_READ) &&
	    (!(attr->perm & BT_GATT_PERM_READ_MASK) || !attr->read)) {
		return BT_ATT_ERR_READ_NOT_PERMITTED;
	}

	if ((mask & BT_GATT_PERM_WRITE) &&
	    (!(attr->perm & BT_GATT_PERM_WRITE_MASK) || !attr->write)) {
		return BT_ATT_ERR_WRITE_NOT_PERMITTED;
	}

	if (IS_ENABLED(CONFIG_BT_CONN_DISABLE_SECURITY)) {
		return 0;
	}

	mask &= attr->perm;
	if (mask & BT_GATT_PERM_AUTHEN_MASK) {
		if (bt_conn_get_security(conn) < BT_SECURITY_L3) {
			return BT_ATT_ERR_AUTHENTICATION;
		}
	}

	if ((mask & BT_GATT_PERM_ENCRYPT_MASK)) {
#if defined(CONFIG_BT_SMP)
		if (!conn->encrypt) {
			return BT_ATT_ERR_INSUFFICIENT_ENCRYPTION;
		}
#else
		return BT_ATT_ERR_INSUFFICIENT_ENCRYPTION;
#endif /* CONFIG_BT_SMP */
	}

	return 0;
}

static void sc_restore_rsp(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, uint8_t err)
{
#if defined(CONFIG_BT_GATT_CACHING)
	struct gatt_cf_cfg *cfg;
#endif

	BT_DBG("err 0x%02x", err);

#if defined(CONFIG_BT_GATT_CACHING)
	/* BLUETOOTH CORE SPECIFICATION Version 5.1 | Vol 3, Part G page 2347:
	 * 2.5.2.1 Robust Caching
	 * A connected client becomes change-aware when...
	 * The client receives and confirms a Service Changed indication.
	 */
	cfg = find_cf_cfg(conn);
	if (cfg && CF_ROBUST_CACHING(cfg)) {
		atomic_set_bit(cfg->flags, CF_CHANGE_AWARE);
		BT_DBG("%s change-aware", bt_addr_le_str(&cfg->peer));
	}
#endif
}

static struct bt_gatt_indicate_params sc_restore_params[CONFIG_BT_MAX_CONN];

static void sc_restore(struct bt_conn *conn)
{
	struct gatt_sc_cfg *cfg;
	uint16_t sc_range[2];
	uint8_t index;

	cfg = find_sc_cfg(conn->id, &conn->le.dst);
	if (!cfg) {
		BT_DBG("no SC data found");
		return;
	}

	if (!(cfg->data.start || cfg->data.end)) {
		return;
	}

	BT_DBG("peer %s start 0x%04x end 0x%04x", bt_addr_le_str(&cfg->peer),
	       cfg->data.start, cfg->data.end);

	sc_range[0] = sys_cpu_to_le16(cfg->data.start);
	sc_range[1] = sys_cpu_to_le16(cfg->data.end);

	index = bt_conn_index(conn);
	sc_restore_params[index].attr = &_1_gatt_svc.attrs[2];
	sc_restore_params[index].func = sc_restore_rsp;
	sc_restore_params[index].data = &sc_range[0];
	sc_restore_params[index].len = sizeof(sc_range);

	if (bt_gatt_indicate(conn, &sc_restore_params[index])) {
		BT_ERR("SC restore indication failed");
	}

	/* Reset config data */
	sc_reset(cfg);
}

struct conn_data {
	struct bt_conn *conn;
	bt_security_t sec;
};

static uint8_t update_ccc(const struct bt_gatt_attr *attr, uint16_t handle,
			  void *user_data)
{
	struct conn_data *data = user_data;
	struct bt_conn *conn = data->conn;
	struct _bt_gatt_ccc *ccc;
	size_t i;
	uint8_t err;

	/* Check attribute user_data must be of type struct _bt_gatt_ccc */
	if (attr->write != bt_gatt_attr_write_ccc) {
		return BT_GATT_ITER_CONTINUE;
	}

	ccc = attr->user_data;

	for (i = 0; i < ARRAY_SIZE(ccc->cfg); i++) {
		struct bt_gatt_ccc_cfg *cfg = &ccc->cfg[i];

		/* Ignore configuration for different peer or not active */
		if (!cfg->value ||
		    !bt_conn_is_peer_addr_le(conn, cfg->id, &cfg->peer)) {
			continue;
		}

		/* Check if attribute requires encryption/authentication */
		err = bt_gatt_check_perm(conn, attr, BT_GATT_PERM_WRITE_MASK);
		if (err) {
			bt_security_t sec;

			if (err == BT_ATT_ERR_WRITE_NOT_PERMITTED) {
				BT_WARN("CCC %p not writable", attr);
				continue;
			}

			sec = BT_SECURITY_L2;

			if (err == BT_ATT_ERR_AUTHENTICATION) {
				sec = BT_SECURITY_L3;
			}

			/* Check if current security is enough */
			if (IS_ENABLED(CONFIG_BT_SMP) &&
			    bt_conn_get_security(conn) < sec) {
				if (data->sec < sec) {
					data->sec = sec;
				}
				continue;
			}
		}

		gatt_ccc_changed(attr, ccc);

		if (IS_ENABLED(CONFIG_BT_GATT_SERVICE_CHANGED) &&
		    ccc == &sc_ccc) {
			sc_restore(conn);
		}

		return BT_GATT_ITER_CONTINUE;
	}

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t disconnected_cb(const struct bt_gatt_attr *attr, uint16_t handle,
			       void *user_data)
{
	struct bt_conn *conn = user_data;
	struct _bt_gatt_ccc *ccc;
	bool value_used;
	size_t i;

	/* Check attribute user_data must be of type struct _bt_gatt_ccc */
	if (attr->write != bt_gatt_attr_write_ccc) {
		return BT_GATT_ITER_CONTINUE;
	}

	ccc = attr->user_data;

	/* If already disabled skip */
	if (!ccc->value) {
		return BT_GATT_ITER_CONTINUE;
	}

	/* Checking if all values are disabled */
	value_used = false;

	for (i = 0; i < ARRAY_SIZE(ccc->cfg); i++) {
		struct bt_gatt_ccc_cfg *cfg = &ccc->cfg[i];

		/* Ignore configurations with disabled value */
		if (!cfg->value) {
			continue;
		}

		if (!bt_conn_is_peer_addr_le(conn, cfg->id, &cfg->peer)) {
			struct bt_conn *tmp;

			/* Skip if there is another peer connected */
			tmp = bt_conn_lookup_addr_le(cfg->id, &cfg->peer);
			if (tmp) {
				if (tmp->state == BT_CONN_CONNECTED) {
					value_used = true;
				}

				bt_conn_unref(tmp);
			}
		} else {
			/* Clear value if not paired */
			if (!bt_addr_le_is_bonded(conn->id, &conn->le.dst)) {
				if (ccc == &sc_ccc) {
					sc_clear(conn);
				}

				clear_ccc_cfg(cfg);
			} else {
				/* Update address in case it has changed */
				bt_addr_le_copy(&cfg->peer, &conn->le.dst);
			}
		}
	}

	/* If all values are now disabled, reset value while disconnected */
	if (!value_used) {
		ccc->value = 0U;
		if (ccc->cfg_changed) {
			ccc->cfg_changed(attr, ccc->value);
		}

		BT_DBG("ccc %p reseted", ccc);
	}

	return BT_GATT_ITER_CONTINUE;
}

bool bt_gatt_is_subscribed(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, uint16_t ccc_value)
{
	const struct _bt_gatt_ccc *ccc;

	__ASSERT(conn, "invalid parameter\n");
	__ASSERT(attr, "invalid parameter\n");

	if (conn->state != BT_CONN_CONNECTED) {
		return false;
	}

	/* Check if attribute is a characteristic declaration */
	if (!bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CHRC)) {
		struct bt_gatt_chrc *chrc = attr->user_data;

		if (!(chrc->properties &
			(BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_INDICATE))) {
			/* Characteristic doesn't support subscription */
			return false;
		}

		attr = bt_gatt_attr_next(attr);
	}

	/* Check if attribute is a characteristic value */
	if (bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CCC) != 0) {
		attr = bt_gatt_attr_next(attr);
	}

	/* Check if the attribute is the CCC Descriptor */
	if (bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CCC) != 0) {
		return false;
	}

	ccc = attr->user_data;

	/* Check if the connection is subscribed */
	for (size_t i = 0; i < BT_GATT_CCC_MAX; i++) {
		const struct bt_gatt_ccc_cfg *cfg = &ccc->cfg[i];

		if (bt_conn_is_peer_addr_le(conn, cfg->id, &cfg->peer) &&
		    (ccc_value & ccc->cfg[i].value)) {
			return true;
		}
	}

	return false;
}

static void gatt_sub_remove(struct bt_conn *conn, struct gatt_sub *sub,
			    sys_snode_t *prev,
			    struct bt_gatt_subscribe_params *params)
{
	if (params) {
		/* Remove subscription from the list*/
		sys_slist_remove(&sub->list, prev, &params->node);
		/* Notify removal */
		params->notify(conn, params, NULL, 0);
	}

	if (sys_slist_is_empty(&sub->list)) {
		/* Reset address if there are no subscription left */
		bt_addr_le_copy(&sub->peer, BT_ADDR_LE_ANY);
	}
}

#if defined(CONFIG_BT_GATT_CLIENT)
static struct gatt_sub *gatt_sub_find(struct bt_conn *conn)
{
	for (int i = 0; i < ARRAY_SIZE(subscriptions); i++) {
		struct gatt_sub *sub = &subscriptions[i];

		if (!conn) {
			if (!bt_addr_le_cmp(&sub->peer, BT_ADDR_LE_ANY)) {
				return sub;
			}
		} else if (bt_conn_is_peer_addr_le(conn, sub->id, &sub->peer)) {
			return sub;
		}
	}

	return NULL;
}

static struct gatt_sub *gatt_sub_add(struct bt_conn *conn)
{
	struct gatt_sub *sub;

	sub = gatt_sub_find(conn);
	if (!sub) {
		sub = gatt_sub_find(NULL);
		if (sub) {
			bt_addr_le_copy(&sub->peer, &conn->le.dst);
			sub->id = conn->id;
		}
	}

	return sub;
}

static struct gatt_sub *gatt_sub_find_by_addr(uint8_t id,
					      const bt_addr_le_t *addr)
{
	for (int i = 0; i < ARRAY_SIZE(subscriptions); i++) {
		struct gatt_sub *sub = &subscriptions[i];

		if (id == sub->id && !bt_addr_le_cmp(&sub->peer, addr)) {
			return sub;
		}
	}

	return NULL;
}

static struct gatt_sub *gatt_sub_add_by_addr(uint8_t id,
					     const bt_addr_le_t *addr)
{
	struct gatt_sub *sub;

	sub = gatt_sub_find_by_addr(id, addr);
	if (!sub) {
		sub = gatt_sub_find(NULL);
		if (sub) {
			bt_addr_le_copy(&sub->peer, addr);
			sub->id = id;
		}
	}

	return sub;
}

void bt_gatt_notification(struct bt_conn *conn, uint16_t handle,
			  const void *data, uint16_t length)
{
	struct bt_gatt_subscribe_params *params, *tmp;
	struct gatt_sub *sub;

	BT_DBG("handle 0x%04x length %u", handle, length);

	sub = gatt_sub_find(conn);
	if (!sub) {
		return;
	}

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&sub->list, params, tmp, node) {
		if (handle != params->value_handle) {
			continue;
		}

		if (params->notify(conn, params, data, length) ==
		    BT_GATT_ITER_STOP) {
			bt_gatt_unsubscribe(conn, params);
		}
	}
}

void bt_gatt_mult_notification(struct bt_conn *conn, const void *data,
			       uint16_t length)
{
	struct bt_gatt_subscribe_params *params, *tmp;
	const struct bt_att_notify_mult *nfy;
	struct net_buf_simple buf;
	struct gatt_sub *sub;

	BT_DBG("length %u", length);

	sub = gatt_sub_find(conn);
	if (!sub) {
		return;
	}

	/* This is fine since there no write operation to the buffer.  */
	net_buf_simple_init_with_data(&buf, (void *)data, length);

	while (buf.len > sizeof(*nfy)) {
		uint16_t handle;
		uint16_t len;

		nfy = net_buf_simple_pull_mem(&buf, sizeof(*nfy));
		handle = sys_cpu_to_le16(nfy->handle);
		len = sys_cpu_to_le16(nfy->len);

		BT_DBG("handle 0x%02x len %u", handle, len);

		if (len > buf.len) {
			BT_ERR("Invalid data len %u > %u", len, length);
			return;
		}

		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&sub->list, params, tmp,
						  node) {
			if (handle != params->value_handle) {
				continue;
			}

			if (params->notify(conn, params, nfy->value, len) ==
			    BT_GATT_ITER_STOP) {
				bt_gatt_unsubscribe(conn, params);
			}
		}

		net_buf_simple_pull_mem(&buf, len);
	}
}

static void gatt_sub_update(struct bt_conn *conn, struct gatt_sub *sub)
{
	if (sub->peer.type == BT_ADDR_LE_PUBLIC) {
		return;
	}

	/* Update address */
	bt_addr_le_copy(&sub->peer, &conn->le.dst);
}

static void remove_subscriptions(struct bt_conn *conn)
{
	struct gatt_sub *sub;
	struct bt_gatt_subscribe_params *params, *tmp;
	sys_snode_t *prev = NULL;

	sub = gatt_sub_find(conn);
	if (!sub) {
		return;
	}

	/* Lookup existing subscriptions */
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&sub->list, params, tmp, node) {
		if (!bt_addr_le_is_bonded(conn->id, &conn->le.dst) ||
		    (atomic_test_bit(params->flags,
				     BT_GATT_SUBSCRIBE_FLAG_VOLATILE))) {
			/* Remove subscription */
			params->value = 0U;
			gatt_sub_remove(conn, sub, prev, params);
		} else {
			gatt_sub_update(conn, sub);
			prev = &params->node;
		}
	}
}

static void gatt_mtu_rsp(struct bt_conn *conn, uint8_t err, const void *pdu,
			 uint16_t length, void *user_data)
{
	struct bt_gatt_exchange_params *params = user_data;

	params->func(conn, err, params);
}

int bt_gatt_exchange_mtu(struct bt_conn *conn,
			 struct bt_gatt_exchange_params *params)
{
	struct bt_att_exchange_mtu_req *req;
	struct net_buf *buf;
	uint16_t mtu;

	__ASSERT(conn, "invalid parameter\n");
	__ASSERT(params && params->func, "invalid parameters\n");

	if (conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	buf = bt_att_create_pdu(conn, BT_ATT_OP_MTU_REQ, sizeof(*req));
	if (!buf) {
		return -ENOMEM;
	}

	mtu = BT_ATT_MTU;

	BT_DBG("Client MTU %u", mtu);

	req = net_buf_add(buf, sizeof(*req));
	req->mtu = sys_cpu_to_le16(mtu);

	return gatt_send(conn, buf, gatt_mtu_rsp, params, NULL);
}

static void gatt_discover_next(struct bt_conn *conn, uint16_t last_handle,
			       struct bt_gatt_discover_params *params)
{
	/* Skip if last_handle is not set */
	if (!last_handle)
		goto discover;

	/* Continue from the last found handle */
	params->start_handle = last_handle;
	if (params->start_handle < UINT16_MAX) {
		params->start_handle++;
	} else {
		goto done;
	}

	/* Stop if over the range or the requests */
	if (params->start_handle > params->end_handle) {
		goto done;
	}

discover:
	/* Discover next range */
	if (!bt_gatt_discover(conn, params)) {
		return;
	}

done:
	params->func(conn, NULL, params);
}

static void gatt_find_type_rsp(struct bt_conn *conn, uint8_t err,
			       const void *pdu, uint16_t length,
			       void *user_data)
{
	const struct bt_att_find_type_rsp *rsp = pdu;
	struct bt_gatt_discover_params *params = user_data;
	uint8_t i;
	uint16_t end_handle = 0U, start_handle;

	BT_DBG("err 0x%02x", err);

	if (err) {
		goto done;
	}

	/* Parse attributes found */
	for (i = 0U; length >= sizeof(rsp->list[i]);
	     i++, length -=  sizeof(rsp->list[i])) {
		struct bt_uuid_16 uuid_svc;
		struct bt_gatt_attr attr = {};
		struct bt_gatt_service_val value;

		start_handle = sys_le16_to_cpu(rsp->list[i].start_handle);
		end_handle = sys_le16_to_cpu(rsp->list[i].end_handle);

		BT_DBG("start_handle 0x%04x end_handle 0x%04x", start_handle,
		       end_handle);

		uuid_svc.uuid.type = BT_UUID_TYPE_16;
		if (params->type == BT_GATT_DISCOVER_PRIMARY) {
			uuid_svc.val = BT_UUID_16(BT_UUID_GATT_PRIMARY)->val;
		} else {
			uuid_svc.val = BT_UUID_16(BT_UUID_GATT_SECONDARY)->val;
		}

		value.end_handle = end_handle;
		value.uuid = params->uuid;

		attr.uuid = &uuid_svc.uuid;
		attr.handle = start_handle;
		attr.user_data = &value;

		if (params->func(conn, &attr, params) == BT_GATT_ITER_STOP) {
			return;
		}
	}

	/* Stop if could not parse the whole PDU */
	if (length > 0) {
		goto done;
	}

	gatt_discover_next(conn, end_handle, params);

	return;
done:
	params->func(conn, NULL, params);
}

static int gatt_find_type(struct bt_conn *conn,
			 struct bt_gatt_discover_params *params)
{
	uint16_t uuid_val;
	struct net_buf *buf;
	struct bt_att_find_type_req *req;

	buf = bt_att_create_pdu(conn, BT_ATT_OP_FIND_TYPE_REQ, sizeof(*req));
	if (!buf) {
		return -ENOMEM;
	}

	req = net_buf_add(buf, sizeof(*req));
	req->start_handle = sys_cpu_to_le16(params->start_handle);
	req->end_handle = sys_cpu_to_le16(params->end_handle);

	if (params->type == BT_GATT_DISCOVER_PRIMARY) {
		uuid_val = BT_UUID_16(BT_UUID_GATT_PRIMARY)->val;
	} else {
		uuid_val = BT_UUID_16(BT_UUID_GATT_SECONDARY)->val;
	}

	req->type = sys_cpu_to_le16(uuid_val);

	BT_DBG("uuid %s start_handle 0x%04x end_handle 0x%04x",
	       bt_uuid_str(params->uuid), params->start_handle,
	       params->end_handle);

	switch (params->uuid->type) {
	case BT_UUID_TYPE_16:
		net_buf_add_le16(buf, BT_UUID_16(params->uuid)->val);
		break;
	case BT_UUID_TYPE_128:
		net_buf_add_mem(buf, BT_UUID_128(params->uuid)->val, 16);
		break;
	default:
		BT_ERR("Unknown UUID type %u", params->uuid->type);
		net_buf_unref(buf);
		return -EINVAL;
	}

	return gatt_send(conn, buf, gatt_find_type_rsp, params, NULL);
}

static void read_included_uuid_cb(struct bt_conn *conn, uint8_t err,
				  const void *pdu, uint16_t length,
				  void *user_data)
{
	struct bt_gatt_discover_params *params = user_data;
	struct bt_gatt_include value;
	struct bt_gatt_attr *attr;
	union {
		struct bt_uuid uuid;
		struct bt_uuid_128 u128;
	} u;

	if (length != 16U) {
		BT_ERR("Invalid data len %u", length);
		params->func(conn, NULL, params);
		return;
	}

	value.start_handle = params->_included.start_handle;
	value.end_handle = params->_included.end_handle;
	value.uuid = &u.uuid;
	u.uuid.type = BT_UUID_TYPE_128;
	memcpy(u.u128.val, pdu, length);

	BT_DBG("handle 0x%04x uuid %s start_handle 0x%04x "
	       "end_handle 0x%04x\n", params->_included.attr_handle,
	       bt_uuid_str(&u.uuid), value.start_handle, value.end_handle);

	/* Skip if UUID is set but doesn't match */
	if (params->uuid && bt_uuid_cmp(&u.uuid, params->uuid)) {
		goto next;
	}

	attr = (&(struct bt_gatt_attr) {
		.uuid = BT_UUID_GATT_INCLUDE,
		.user_data = &value, });
	attr->handle = params->_included.attr_handle;

	if (params->func(conn, attr, params) == BT_GATT_ITER_STOP) {
		return;
	}
next:
	gatt_discover_next(conn, params->start_handle, params);

	return;
}

static int read_included_uuid(struct bt_conn *conn,
			      struct bt_gatt_discover_params *params)
{
	struct net_buf *buf;
	struct bt_att_read_req *req;

	buf = bt_att_create_pdu(conn, BT_ATT_OP_READ_REQ, sizeof(*req));
	if (!buf) {
		return -ENOMEM;
	}

	req = net_buf_add(buf, sizeof(*req));
	req->handle = sys_cpu_to_le16(params->_included.start_handle);

	BT_DBG("handle 0x%04x", params->_included.start_handle);

	return gatt_send(conn, buf, read_included_uuid_cb, params, NULL);
}

static uint16_t parse_include(struct bt_conn *conn, const void *pdu,
			   struct bt_gatt_discover_params *params,
			   uint16_t length)
{
	const struct bt_att_read_type_rsp *rsp = pdu;
	uint16_t handle = 0U;
	struct bt_gatt_include value;
	union {
		struct bt_uuid uuid;
		struct bt_uuid_16 u16;
		struct bt_uuid_128 u128;
	} u;

	/* Data can be either in UUID16 or UUID128 */
	switch (rsp->len) {
	case 8: /* UUID16 */
		u.uuid.type = BT_UUID_TYPE_16;
		break;
	case 6: /* UUID128 */
		/* BLUETOOTH SPECIFICATION Version 4.2 [Vol 3, Part G] page 550
		 * To get the included service UUID when the included service
		 * uses a 128-bit UUID, the Read Request is used.
		 */
		u.uuid.type = BT_UUID_TYPE_128;
		break;
	default:
		BT_ERR("Invalid data len %u", rsp->len);
		goto done;
	}

	/* Parse include found */
	for (length--, pdu = rsp->data; length >= rsp->len;
	     length -= rsp->len, pdu = (const uint8_t *)pdu + rsp->len) {
		struct bt_gatt_attr *attr;
		const struct bt_att_data *data = pdu;
		struct gatt_incl *incl = (void *)data->value;

		handle = sys_le16_to_cpu(data->handle);
		/* Handle 0 is invalid */
		if (!handle) {
			goto done;
		}

		/* Convert include data, bt_gatt_incl and gatt_incl
		 * have different formats so the conversion have to be done
		 * field by field.
		 */
		value.start_handle = sys_le16_to_cpu(incl->start_handle);
		value.end_handle = sys_le16_to_cpu(incl->end_handle);

		switch (u.uuid.type) {
		case BT_UUID_TYPE_16:
			value.uuid = &u.uuid;
			u.u16.val = sys_le16_to_cpu(incl->uuid16);
			break;
		case BT_UUID_TYPE_128:
			params->_included.attr_handle = handle;
			params->_included.start_handle = value.start_handle;
			params->_included.end_handle = value.end_handle;

			return read_included_uuid(conn, params);
		}

		BT_DBG("handle 0x%04x uuid %s start_handle 0x%04x "
		       "end_handle 0x%04x\n", handle, bt_uuid_str(&u.uuid),
		       value.start_handle, value.end_handle);

		/* Skip if UUID is set but doesn't match */
		if (params->uuid && bt_uuid_cmp(&u.uuid, params->uuid)) {
			continue;
		}

		attr = (&(struct bt_gatt_attr) {
			.uuid = BT_UUID_GATT_INCLUDE,
			.user_data = &value, });
		attr->handle = handle;

		if (params->func(conn, attr, params) == BT_GATT_ITER_STOP) {
			return 0;
		}
	}

	/* Whole PDU read without error */
	if (length == 0U && handle) {
		return handle;
	}

done:
	params->func(conn, NULL, params);
	return 0;
}

#define BT_GATT_CHRC(_uuid, _handle, _props)				\
	BT_GATT_ATTRIBUTE(BT_UUID_GATT_CHRC, BT_GATT_PERM_READ,		\
			  bt_gatt_attr_read_chrc, NULL,			\
			  (&(struct bt_gatt_chrc) { .uuid = _uuid,	\
						    .value_handle = _handle,  \
						    .properties = _props, }))

static uint16_t parse_characteristic(struct bt_conn *conn, const void *pdu,
				  struct bt_gatt_discover_params *params,
				  uint16_t length)
{
	const struct bt_att_read_type_rsp *rsp = pdu;
	uint16_t handle = 0U;
	union {
		struct bt_uuid uuid;
		struct bt_uuid_16 u16;
		struct bt_uuid_128 u128;
	} u;

	/* Data can be either in UUID16 or UUID128 */
	switch (rsp->len) {
	case 7: /* UUID16 */
		u.uuid.type = BT_UUID_TYPE_16;
		break;
	case 21: /* UUID128 */
		u.uuid.type = BT_UUID_TYPE_128;
		break;
	default:
		BT_ERR("Invalid data len %u", rsp->len);
		goto done;
	}

	/* Parse characteristics found */
	for (length--, pdu = rsp->data; length >= rsp->len;
	     length -= rsp->len, pdu = (const uint8_t *)pdu + rsp->len) {
		struct bt_gatt_attr *attr;
		const struct bt_att_data *data = pdu;
		struct gatt_chrc *chrc = (void *)data->value;

		handle = sys_le16_to_cpu(data->handle);
		/* Handle 0 is invalid */
		if (!handle) {
			goto done;
		}

		switch (u.uuid.type) {
		case BT_UUID_TYPE_16:
			u.u16.val = sys_le16_to_cpu(chrc->uuid16);
			break;
		case BT_UUID_TYPE_128:
			memcpy(u.u128.val, chrc->uuid, sizeof(chrc->uuid));
			break;
		}

		BT_DBG("handle 0x%04x uuid %s properties 0x%02x", handle,
		       bt_uuid_str(&u.uuid), chrc->properties);

		/* Skip if UUID is set but doesn't match */
		if (params->uuid && bt_uuid_cmp(&u.uuid, params->uuid)) {
			continue;
		}

		attr = (&(struct bt_gatt_attr)BT_GATT_CHRC(&u.uuid,
							   chrc->value_handle,
							   chrc->properties));
		attr->handle = handle;

		if (params->func(conn, attr, params) == BT_GATT_ITER_STOP) {
			return 0;
		}
	}

	/* Whole PDU read without error */
	if (length == 0U && handle) {
		return handle;
	}

done:
	params->func(conn, NULL, params);
	return 0;
}

static void gatt_read_type_rsp(struct bt_conn *conn, uint8_t err,
			       const void *pdu, uint16_t length,
			       void *user_data)
{
	struct bt_gatt_discover_params *params = user_data;
	uint16_t handle;

	BT_DBG("err 0x%02x", err);

	if (err) {
		params->func(conn, NULL, params);
		return;
	}

	if (params->type == BT_GATT_DISCOVER_INCLUDE) {
		handle = parse_include(conn, pdu, params, length);
	} else {
		handle = parse_characteristic(conn, pdu, params, length);
	}

	if (!handle) {
		return;
	}

	gatt_discover_next(conn, handle, params);
}

static int gatt_read_type(struct bt_conn *conn,
			  struct bt_gatt_discover_params *params)
{
	struct net_buf *buf;
	struct bt_att_read_type_req *req;

	buf = bt_att_create_pdu(conn, BT_ATT_OP_READ_TYPE_REQ, sizeof(*req));
	if (!buf) {
		return -ENOMEM;
	}

	req = net_buf_add(buf, sizeof(*req));
	req->start_handle = sys_cpu_to_le16(params->start_handle);
	req->end_handle = sys_cpu_to_le16(params->end_handle);

	if (params->type == BT_GATT_DISCOVER_INCLUDE) {
		net_buf_add_le16(buf, BT_UUID_16(BT_UUID_GATT_INCLUDE)->val);
	} else {
		net_buf_add_le16(buf, BT_UUID_16(BT_UUID_GATT_CHRC)->val);
	}

	BT_DBG("start_handle 0x%04x end_handle 0x%04x", params->start_handle,
	       params->end_handle);

	return gatt_send(conn, buf, gatt_read_type_rsp, params, NULL);
}

static uint16_t parse_service(struct bt_conn *conn, const void *pdu,
				  struct bt_gatt_discover_params *params,
				  uint16_t length)
{
	const struct bt_att_read_group_rsp *rsp = pdu;
	uint16_t start_handle, end_handle = 0U;
	union {
		struct bt_uuid uuid;
		struct bt_uuid_16 u16;
		struct bt_uuid_128 u128;
	} u;

	/* Data can be either in UUID16 or UUID128 */
	switch (rsp->len) {
	case 6: /* UUID16 */
		u.uuid.type = BT_UUID_TYPE_16;
		break;
	case 20: /* UUID128 */
		u.uuid.type = BT_UUID_TYPE_128;
		break;
	default:
		BT_ERR("Invalid data len %u", rsp->len);
		goto done;
	}

	/* Parse services found */
	for (length--, pdu = rsp->data; length >= rsp->len;
	     length -= rsp->len, pdu = (const uint8_t *)pdu + rsp->len) {
		struct bt_uuid_16 uuid_svc;
		struct bt_gatt_attr attr = {};
		struct bt_gatt_service_val value;
		const struct bt_att_group_data *data = pdu;

		start_handle = sys_le16_to_cpu(data->start_handle);
		if (!start_handle) {
			goto done;
		}

		end_handle = sys_le16_to_cpu(data->end_handle);
		if (!end_handle || end_handle < start_handle) {
			goto done;
		}

		switch (u.uuid.type) {
		case BT_UUID_TYPE_16:
			memcpy(&u.u16.val, data->value, sizeof(u.u16.val));
			u.u16.val = sys_le16_to_cpu(u.u16.val);
			break;
		case BT_UUID_TYPE_128:
			memcpy(u.u128.val, data->value, sizeof(u.u128.val));
			break;
		}

		BT_DBG("start_handle 0x%04x end_handle 0x%04x uuid %s",
		       start_handle, end_handle, bt_uuid_str(&u.uuid));

		uuid_svc.uuid.type = BT_UUID_TYPE_16;
		if (params->type == BT_GATT_DISCOVER_PRIMARY) {
			uuid_svc.val = BT_UUID_16(BT_UUID_GATT_PRIMARY)->val;
		} else {
			uuid_svc.val = BT_UUID_16(BT_UUID_GATT_SECONDARY)->val;
		}

		value.end_handle = end_handle;
		value.uuid = &u.uuid;

		attr.uuid = &uuid_svc.uuid;
		attr.handle = start_handle;
		attr.user_data = &value;

		if (params->func(conn, &attr, params) == BT_GATT_ITER_STOP) {
			return 0;
		}
	}

	/* Whole PDU read without error */
	if (length == 0U && end_handle) {
		return end_handle;
	}

done:
	params->func(conn, NULL, params);
	return 0;
}

static void gatt_read_group_rsp(struct bt_conn *conn, uint8_t err,
				const void *pdu, uint16_t length,
				void *user_data)
{
	struct bt_gatt_discover_params *params = user_data;
	uint16_t handle;

	BT_DBG("err 0x%02x", err);

	if (err) {
		params->func(conn, NULL, params);
		return;
	}

	handle = parse_service(conn, pdu, params, length);
	if (!handle) {
		return;
	}

	gatt_discover_next(conn, handle, params);
}

static int gatt_read_group(struct bt_conn *conn,
			   struct bt_gatt_discover_params *params)
{
	struct net_buf *buf;
	struct bt_att_read_group_req *req;

	buf = bt_att_create_pdu(conn, BT_ATT_OP_READ_GROUP_REQ, sizeof(*req));
	if (!buf) {
		return -ENOMEM;
	}

	req = net_buf_add(buf, sizeof(*req));
	req->start_handle = sys_cpu_to_le16(params->start_handle);
	req->end_handle = sys_cpu_to_le16(params->end_handle);

	if (params->type == BT_GATT_DISCOVER_PRIMARY) {
		net_buf_add_le16(buf, BT_UUID_16(BT_UUID_GATT_PRIMARY)->val);
	} else {
		net_buf_add_le16(buf, BT_UUID_16(BT_UUID_GATT_SECONDARY)->val);
	}

	BT_DBG("start_handle 0x%04x end_handle 0x%04x", params->start_handle,
	       params->end_handle);

	return gatt_send(conn, buf, gatt_read_group_rsp, params, NULL);
}

static void gatt_find_info_rsp(struct bt_conn *conn, uint8_t err,
			       const void *pdu, uint16_t length,
			       void *user_data)
{
	const struct bt_att_find_info_rsp *rsp = pdu;
	struct bt_gatt_discover_params *params = user_data;
	uint16_t handle = 0U;
	uint16_t len;
	union {
		const struct bt_att_info_16 *i16;
		const struct bt_att_info_128 *i128;
	} info;
	union {
		struct bt_uuid uuid;
		struct bt_uuid_16 u16;
		struct bt_uuid_128 u128;
	} u;
	int i;
	bool skip = false;

	BT_DBG("err 0x%02x", err);

	if (err) {
		goto done;
	}

	/* Data can be either in UUID16 or UUID128 */
	switch (rsp->format) {
	case BT_ATT_INFO_16:
		u.uuid.type = BT_UUID_TYPE_16;
		len = sizeof(*info.i16);
		break;
	case BT_ATT_INFO_128:
		u.uuid.type = BT_UUID_TYPE_128;
		len = sizeof(*info.i128);
		break;
	default:
		BT_ERR("Invalid format %u", rsp->format);
		goto done;
	}

	length--;

	/* Check if there is a least one descriptor in the response */
	if (length < len) {
		goto done;
	}

	/* Parse descriptors found */
	for (i = length / len, pdu = rsp->info; i != 0;
	     i--, pdu = (const uint8_t *)pdu + len) {
		struct bt_gatt_attr *attr;

		info.i16 = pdu;
		handle = sys_le16_to_cpu(info.i16->handle);

		if (skip) {
			skip = false;
			continue;
		}

		switch (u.uuid.type) {
		case BT_UUID_TYPE_16:
			u.u16.val = sys_le16_to_cpu(info.i16->uuid);
			break;
		case BT_UUID_TYPE_128:
			memcpy(u.u128.val, info.i128->uuid, 16);
			break;
		}

		BT_DBG("handle 0x%04x uuid %s", handle, bt_uuid_str(&u.uuid));

		/* Skip if UUID is set but doesn't match */
		if (params->uuid && bt_uuid_cmp(&u.uuid, params->uuid)) {
			continue;
		}

		if (params->type == BT_GATT_DISCOVER_DESCRIPTOR) {
			/* Skip attributes that are not considered
			 * descriptors.
			 */
			if (!bt_uuid_cmp(&u.uuid, BT_UUID_GATT_PRIMARY) ||
			    !bt_uuid_cmp(&u.uuid, BT_UUID_GATT_SECONDARY) ||
			    !bt_uuid_cmp(&u.uuid, BT_UUID_GATT_INCLUDE)) {
				continue;
			}

			/* If Characteristic Declaration skip ahead as the next
			 * entry must be its value.
			 */
			if (!bt_uuid_cmp(&u.uuid, BT_UUID_GATT_CHRC)) {
				skip = true;
				continue;
			}
		}

		attr = (&(struct bt_gatt_attr)
			BT_GATT_DESCRIPTOR(&u.uuid, 0, NULL, NULL, NULL));
		attr->handle = handle;

		if (params->func(conn, attr, params) == BT_GATT_ITER_STOP) {
			return;
		}
	}

	gatt_discover_next(conn, handle, params);

	return;

done:
	params->func(conn, NULL, params);
}

static int gatt_find_info(struct bt_conn *conn,
			  struct bt_gatt_discover_params *params)
{
	struct net_buf *buf;
	struct bt_att_find_info_req *req;

	buf = bt_att_create_pdu(conn, BT_ATT_OP_FIND_INFO_REQ, sizeof(*req));
	if (!buf) {
		return -ENOMEM;
	}

	req = net_buf_add(buf, sizeof(*req));
	req->start_handle = sys_cpu_to_le16(params->start_handle);
	req->end_handle = sys_cpu_to_le16(params->end_handle);

	BT_DBG("start_handle 0x%04x end_handle 0x%04x", params->start_handle,
	       params->end_handle);

	return gatt_send(conn, buf, gatt_find_info_rsp, params, NULL);
}

int bt_gatt_discover(struct bt_conn *conn,
		     struct bt_gatt_discover_params *params)
{
	__ASSERT(conn, "invalid parameters\n");
	__ASSERT(params && params->func, "invalid parameters\n");
	__ASSERT((params->start_handle && params->end_handle),
		 "invalid parameters\n");
	__ASSERT((params->start_handle <= params->end_handle),
		 "invalid parameters\n");

	if (conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	switch (params->type) {
	case BT_GATT_DISCOVER_PRIMARY:
	case BT_GATT_DISCOVER_SECONDARY:
		if (params->uuid) {
			return gatt_find_type(conn, params);
		}
		return gatt_read_group(conn, params);
	case BT_GATT_DISCOVER_INCLUDE:
	case BT_GATT_DISCOVER_CHARACTERISTIC:
		return gatt_read_type(conn, params);
	case BT_GATT_DISCOVER_DESCRIPTOR:
		/* Only descriptors can be filtered */
		if (params->uuid &&
		    (!bt_uuid_cmp(params->uuid, BT_UUID_GATT_PRIMARY) ||
		     !bt_uuid_cmp(params->uuid, BT_UUID_GATT_SECONDARY) ||
		     !bt_uuid_cmp(params->uuid, BT_UUID_GATT_INCLUDE) ||
		     !bt_uuid_cmp(params->uuid, BT_UUID_GATT_CHRC))) {
			return -EINVAL;
		}
		__fallthrough;
	case BT_GATT_DISCOVER_ATTRIBUTE:
		return gatt_find_info(conn, params);
	default:
		BT_ERR("Invalid discovery type: %u", params->type);
	}

	return -EINVAL;
}

static void parse_read_by_uuid(struct bt_conn *conn,
			       struct bt_gatt_read_params *params,
			       const void *pdu, uint16_t length)
{
	const struct bt_att_read_type_rsp *rsp = pdu;

	/* Parse values found */
	for (length--, pdu = rsp->data; length;
	     length -= rsp->len, pdu = (const uint8_t *)pdu + rsp->len) {
		const struct bt_att_data *data = pdu;
		uint16_t handle;
		uint16_t len;

		handle = sys_le16_to_cpu(data->handle);

		/* Handle 0 is invalid */
		if (!handle) {
			BT_ERR("Invalid handle");
			return;
		}

		len = rsp->len > length ? length - 2 : rsp->len - 2;

		BT_DBG("handle 0x%04x len %u value %u", handle, rsp->len, len);

		/* Update start_handle */
		params->by_uuid.start_handle = handle;

		if (params->func(conn, 0, params, data->value, len) ==
		    BT_GATT_ITER_STOP) {
			return;
		}

		/* Check if long attribute */
		if (rsp->len > length) {
			break;
		}

		/* Stop if it's the last handle to be read */
		if (params->by_uuid.start_handle == params->by_uuid.end_handle) {
			params->func(conn, 0, params, NULL, 0);
			return;
		}

		params->by_uuid.start_handle++;
	}

	/* Continue reading the attributes */
	if (bt_gatt_read(conn, params) < 0) {
		params->func(conn, BT_ATT_ERR_UNLIKELY, params, NULL, 0);
	}
}

static void gatt_read_rsp(struct bt_conn *conn, uint8_t err, const void *pdu,
			  uint16_t length, void *user_data)
{
	struct bt_gatt_read_params *params = user_data;

	BT_DBG("err 0x%02x", err);

	if (err || !length) {
		params->func(conn, err, params, NULL, 0);
		return;
	}

	if (!params->handle_count) {
		parse_read_by_uuid(conn, params, pdu, length);
		return;
	}

	if (params->func(conn, 0, params, pdu, length) == BT_GATT_ITER_STOP) {
		return;
	}

	/*
	 * Core Spec 4.2, Vol. 3, Part G, 4.8.1
	 * If the Characteristic Value is greater than (ATT_MTU - 1) octets
	 * in length, the Read Long Characteristic Value procedure may be used
	 * if the rest of the Characteristic Value is required.
	 */
	if (length < (bt_att_get_mtu(conn) - 1)) {
		params->func(conn, 0, params, NULL, 0);
		return;
	}

	params->single.offset += length;

	/* Continue reading the attribute */
	if (bt_gatt_read(conn, params) < 0) {
		params->func(conn, BT_ATT_ERR_UNLIKELY, params, NULL, 0);
	}
}

static int gatt_read_blob(struct bt_conn *conn,
			  struct bt_gatt_read_params *params)
{
	struct net_buf *buf;
	struct bt_att_read_blob_req *req;

	buf = bt_att_create_pdu(conn, BT_ATT_OP_READ_BLOB_REQ, sizeof(*req));
	if (!buf) {
		return -ENOMEM;
	}

	req = net_buf_add(buf, sizeof(*req));
	req->handle = sys_cpu_to_le16(params->single.handle);
	req->offset = sys_cpu_to_le16(params->single.offset);

	BT_DBG("handle 0x%04x offset 0x%04x", params->single.handle,
	       params->single.offset);

	return gatt_send(conn, buf, gatt_read_rsp, params, NULL);
}

static int gatt_read_uuid(struct bt_conn *conn,
			  struct bt_gatt_read_params *params)
{
	struct net_buf *buf;
	struct bt_att_read_type_req *req;

	buf = bt_att_create_pdu(conn, BT_ATT_OP_READ_TYPE_REQ, sizeof(*req));
	if (!buf) {
		return -ENOMEM;
	}

	req = net_buf_add(buf, sizeof(*req));
	req->start_handle = sys_cpu_to_le16(params->by_uuid.start_handle);
	req->end_handle = sys_cpu_to_le16(params->by_uuid.end_handle);

	if (params->by_uuid.uuid->type == BT_UUID_TYPE_16) {
		net_buf_add_le16(buf, BT_UUID_16(params->by_uuid.uuid)->val);
	} else {
		net_buf_add_mem(buf, BT_UUID_128(params->by_uuid.uuid)->val, 16);
	}

	BT_DBG("start_handle 0x%04x end_handle 0x%04x uuid %s",
		params->by_uuid.start_handle, params->by_uuid.end_handle,
		bt_uuid_str(params->by_uuid.uuid));

	return gatt_send(conn, buf, gatt_read_rsp, params, NULL);
}

#if defined(CONFIG_BT_GATT_READ_MULTIPLE)
static void gatt_read_mult_rsp(struct bt_conn *conn, uint8_t err, const void *pdu,
			       uint16_t length, void *user_data)
{
	struct bt_gatt_read_params *params = user_data;

	BT_DBG("err 0x%02x", err);

	if (err || !length) {
		params->func(conn, err, params, NULL, 0);
		return;
	}

	params->func(conn, 0, params, pdu, length);

	/* mark read as complete since read multiple is single response */
	params->func(conn, 0, params, NULL, 0);
}

static int gatt_read_mult(struct bt_conn *conn,
			  struct bt_gatt_read_params *params)
{
	struct net_buf *buf;
	uint8_t i;

	buf = bt_att_create_pdu(conn, BT_ATT_OP_READ_MULT_REQ,
				params->handle_count * sizeof(uint16_t));
	if (!buf) {
		return -ENOMEM;
	}

	for (i = 0U; i < params->handle_count; i++) {
		net_buf_add_le16(buf, params->handles[i]);
	}

	return gatt_send(conn, buf, gatt_read_mult_rsp, params, NULL);
}

#if defined(CONFIG_BT_EATT)
static void gatt_read_mult_vl_rsp(struct bt_conn *conn, uint8_t err,
				  const void *pdu, uint16_t length,
				  void *user_data)
{
	struct bt_gatt_read_params *params = user_data;
	const struct bt_att_read_mult_vl_rsp *rsp;
	struct net_buf_simple buf;

	BT_DBG("err 0x%02x", err);

	if (err || !length) {
		if (err == BT_ATT_ERR_NOT_SUPPORTED) {
			gatt_read_mult(conn, params);
		}
		params->func(conn, err, params, NULL, 0);
		return;
	}

	net_buf_simple_init_with_data(&buf, (void *)pdu, length);

	while (buf.len >= sizeof(*rsp)) {
		uint16_t len;

		rsp = net_buf_simple_pull_mem(&buf, sizeof(*rsp));
		len = sys_le16_to_cpu(rsp->len);

		/* If a Length Value Tuple is truncated, then the amount of
		 * Attribute Value will be less than the value of the Value
		 * Length field.
		 */
		if (len > buf.len) {
			len = buf.len;
		}

		params->func(conn, 0, params, rsp->value, len);

		net_buf_simple_pull_mem(&buf, len);
	}

	/* mark read as complete since read multiple is single response */
	params->func(conn, 0, params, NULL, 0);
}

static int gatt_read_mult_vl(struct bt_conn *conn,
			      struct bt_gatt_read_params *params)
{
	struct net_buf *buf;
	uint8_t i;

	buf = bt_att_create_pdu(conn, BT_ATT_OP_READ_MULT_VL_REQ,
				params->handle_count * sizeof(uint16_t));
	if (!buf) {
		return -ENOMEM;
	}

	for (i = 0U; i < params->handle_count; i++) {
		net_buf_add_le16(buf, params->handles[i]);
	}

	return gatt_send(conn, buf, gatt_read_mult_vl_rsp, params, NULL);
}
#endif /* CONFIG_BT_EATT */

#else
static int gatt_read_mult(struct bt_conn *conn,
			      struct bt_gatt_read_params *params)
{
	return -ENOTSUP;
}

static int gatt_read_mult_vl(struct bt_conn *conn,
			      struct bt_gatt_read_params *params)
{
	return -ENOTSUP;
}
#endif /* CONFIG_BT_GATT_READ_MULTIPLE */

int bt_gatt_read(struct bt_conn *conn, struct bt_gatt_read_params *params)
{
	struct net_buf *buf;
	struct bt_att_read_req *req;

	__ASSERT(conn, "invalid parameters\n");
	__ASSERT(params && params->func, "invalid parameters\n");

	if (conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	if (params->handle_count == 0) {
		return gatt_read_uuid(conn, params);
	}

	if (params->handle_count > 1) {
#if defined(CONFIG_BT_EATT)
		return gatt_read_mult_vl(conn, params);
#else
		return gatt_read_mult(conn, params);
#endif /* CONFIG_BT_EATT */
	}

	if (params->single.offset) {
		return gatt_read_blob(conn, params);
	}

	buf = bt_att_create_pdu(conn, BT_ATT_OP_READ_REQ, sizeof(*req));
	if (!buf) {
		return -ENOMEM;
	}

	req = net_buf_add(buf, sizeof(*req));
	req->handle = sys_cpu_to_le16(params->single.handle);

	BT_DBG("handle 0x%04x", params->single.handle);

	return gatt_send(conn, buf, gatt_read_rsp, params, NULL);
}

static void gatt_write_rsp(struct bt_conn *conn, uint8_t err, const void *pdu,
			   uint16_t length, void *user_data)
{
	struct bt_gatt_write_params *params = user_data;

	BT_DBG("err 0x%02x", err);

	params->func(conn, err, params);
}

int bt_gatt_write_without_response_cb(struct bt_conn *conn, uint16_t handle,
				      const void *data, uint16_t length, bool sign,
				      bt_gatt_complete_func_t func,
				      void *user_data)
{
	struct net_buf *buf;
	struct bt_att_write_cmd *cmd;
	size_t write;

	__ASSERT(conn, "invalid parameters\n");
	__ASSERT(handle, "invalid parameters\n");

	if (conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

#if defined(CONFIG_BT_SMP)
	if (conn->encrypt) {
		/* Don't need to sign if already encrypted */
		sign = false;
	}
#endif

	if (sign) {
		buf = bt_att_create_pdu(conn, BT_ATT_OP_SIGNED_WRITE_CMD,
					sizeof(*cmd) + length + 12);
	} else {
		buf = bt_att_create_pdu(conn, BT_ATT_OP_WRITE_CMD,
					sizeof(*cmd) + length);
	}
	if (!buf) {
		return -ENOMEM;
	}

	cmd = net_buf_add(buf, sizeof(*cmd));
	cmd->handle = sys_cpu_to_le16(handle);

	write = net_buf_append_bytes(buf, length, data, K_NO_WAIT, NULL, NULL);
	if (write != length) {
		BT_WARN("Unable to allocate length %u: only %zu written",
			length, write);
		net_buf_unref(buf);
		return -ENOMEM;
	}

	BT_DBG("handle 0x%04x length %u", handle, length);

	return bt_att_send(conn, buf, func, user_data);
}

static int gatt_exec_write(struct bt_conn *conn,
			   struct bt_gatt_write_params *params)
{
	struct net_buf *buf;
	struct bt_att_exec_write_req *req;

	buf = bt_att_create_pdu(conn, BT_ATT_OP_EXEC_WRITE_REQ, sizeof(*req));
	if (!buf) {
		return -ENOMEM;
	}

	req = net_buf_add(buf, sizeof(*req));
	req->flags = BT_ATT_FLAG_EXEC;

	BT_DBG("");

	return gatt_send(conn, buf, gatt_write_rsp, params, NULL);
}

static void gatt_prepare_write_rsp(struct bt_conn *conn, uint8_t err,
				   const void *pdu, uint16_t length,
				   void *user_data)
{
	struct bt_gatt_write_params *params = user_data;

	BT_DBG("err 0x%02x", err);


	/* Don't continue in case of error */
	if (err) {
		params->func(conn, err, params);
		return;
	}

	/* If there is no more data execute */
	if (!params->length) {
		gatt_exec_write(conn, params);
		return;
	}

	/* Write next chunk */
	bt_gatt_write(conn, params);
}

static int gatt_prepare_write(struct bt_conn *conn,
			      struct bt_gatt_write_params *params)
{
	struct net_buf *buf;
	struct bt_att_prepare_write_req *req;
	uint16_t len;
	size_t write;

	len = MIN(params->length, bt_att_get_mtu(conn) - sizeof(*req) - 1);

	buf = bt_att_create_pdu(conn, BT_ATT_OP_PREPARE_WRITE_REQ,
				sizeof(*req) + len);
	if (!buf) {
		return -ENOMEM;
	}

	req = net_buf_add(buf, sizeof(*req));
	req->handle = sys_cpu_to_le16(params->handle);
	req->offset = sys_cpu_to_le16(params->offset);

	/* Append as much as possible */
	write = net_buf_append_bytes(buf, len, params->data, K_NO_WAIT,
				     NULL, NULL);

	/* Update params */
	params->offset += write;
	params->data = (const uint8_t *)params->data + len;
	params->length -= write;

	BT_DBG("handle 0x%04x offset %u len %u", params->handle, params->offset,
	       params->length);

	return gatt_send(conn, buf, gatt_prepare_write_rsp, params, NULL);
}

int bt_gatt_write(struct bt_conn *conn, struct bt_gatt_write_params *params)
{
	struct net_buf *buf;
	struct bt_att_write_req *req;
	size_t write;

	__ASSERT(conn, "invalid parameters\n");
	__ASSERT(params && params->func, "invalid parameters\n");
	__ASSERT(params->handle, "invalid parameters\n");

	if (conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	/* Use Prepare Write if offset is set or Long Write is required */
	if (params->offset ||
	    params->length > (bt_att_get_mtu(conn) - sizeof(*req) - 1)) {
		return gatt_prepare_write(conn, params);
	}

	buf = bt_att_create_pdu(conn, BT_ATT_OP_WRITE_REQ,
				sizeof(*req) + params->length);
	if (!buf) {
		return -ENOMEM;
	}

	req = net_buf_add(buf, sizeof(*req));
	req->handle = sys_cpu_to_le16(params->handle);

	write = net_buf_append_bytes(buf, params->length, params->data,
				     K_NO_WAIT, NULL, NULL);
	if (write != params->length) {
		net_buf_unref(buf);
		return -ENOMEM;
	}

	BT_DBG("handle 0x%04x length %u", params->handle, params->length);

	return gatt_send(conn, buf, gatt_write_rsp, params, NULL);
}

static void gatt_write_ccc_rsp(struct bt_conn *conn, uint8_t err,
			       const void *pdu, uint16_t length,
			       void *user_data)
{
	struct bt_gatt_subscribe_params *params = user_data;

	BT_DBG("err 0x%02x", err);

	atomic_clear_bit(params->flags, BT_GATT_SUBSCRIBE_FLAG_WRITE_PENDING);

	/* if write to CCC failed we remove subscription and notify app */
	if (err) {
		struct gatt_sub *sub;
		sys_snode_t *node, *tmp, *prev = NULL;

		sub = gatt_sub_find(conn);
		if (!sub) {
			return;
		}

		SYS_SLIST_FOR_EACH_NODE_SAFE(&sub->list, node, tmp) {
			if (node == &params->node) {
				gatt_sub_remove(conn, sub, tmp, params);
				break;
			}

			prev = node;
		}
	} else if (!params->value) {
		/* Notify with NULL data to complete unsubscribe */
		params->notify(conn, params, NULL, 0);
	}

	if (params->write) {
		params->write(conn, err, NULL);
	}
}

static int gatt_write_ccc(struct bt_conn *conn, uint16_t handle, uint16_t value,
			  bt_att_func_t func,
			  struct bt_gatt_subscribe_params *params)
{
	struct net_buf *buf;
	struct bt_att_write_req *req;

	buf = bt_att_create_pdu(conn, BT_ATT_OP_WRITE_REQ,
				sizeof(*req) + sizeof(uint16_t));
	if (!buf) {
		return -ENOMEM;
	}

	req = net_buf_add(buf, sizeof(*req));
	req->handle = sys_cpu_to_le16(handle);
	net_buf_add_le16(buf, value);

	BT_DBG("handle 0x%04x value 0x%04x", handle, value);

	atomic_set_bit(params->flags, BT_GATT_SUBSCRIBE_FLAG_WRITE_PENDING);

	return gatt_send(conn, buf, func, params, NULL);
}

#if defined(CONFIG_BT_GATT_AUTO_DISCOVER_CCC)
static uint8_t gatt_ccc_discover_cb(struct bt_conn *conn,
				    const struct bt_gatt_attr *attr,
				    struct bt_gatt_discover_params *params)
{
	struct bt_gatt_subscribe_params *sub_params = params->sub_params;

	if (!attr) {
		memset(params, 0, sizeof(*params));
		sub_params->notify(conn, sub_params, NULL, 0);
		return BT_GATT_ITER_STOP;
	}

	if (params->type == BT_GATT_DISCOVER_DESCRIPTOR) {
		memset(params, 0, sizeof(*params));
		sub_params->ccc_handle = attr->handle;

		if (bt_gatt_subscribe(conn, sub_params)) {
			sub_params->notify(conn, sub_params, NULL, 0);
		}
		/* else if no error occurred, then `bt_gatt_subscribe` will
		 * call the notify function once subscribed.
		 */

		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_CONTINUE;
}

static int gatt_ccc_discover(struct bt_conn *conn,
			     struct bt_gatt_subscribe_params *params)
{
	int err;
	static struct bt_uuid_16 ccc_uuid = BT_UUID_INIT_16(0);

	memcpy(&ccc_uuid, BT_UUID_GATT_CCC, sizeof(ccc_uuid));
	memset(params->disc_params, 0, sizeof(*params->disc_params));

	params->disc_params->sub_params = params;
	params->disc_params->uuid = &ccc_uuid.uuid;
	params->disc_params->type = BT_GATT_DISCOVER_DESCRIPTOR;
	params->disc_params->start_handle = params->value_handle;
	params->disc_params->end_handle = params->end_handle;
	params->disc_params->func = gatt_ccc_discover_cb;

	err = bt_gatt_discover(conn, params->disc_params);
	if (err) {
		BT_DBG("CCC Discovery failed (err %d)", err);
		return err;
	}
	return 0;

}
#endif /* CONFIG_BT_GATT_AUTO_DISCOVER_CCC */

int bt_gatt_subscribe(struct bt_conn *conn,
		      struct bt_gatt_subscribe_params *params)
{
	struct gatt_sub *sub;
	struct bt_gatt_subscribe_params *tmp;
	bool has_subscription = false;

	__ASSERT(conn, "invalid parameters\n");
	__ASSERT(params && params->notify,  "invalid parameters\n");
	__ASSERT(params->value, "invalid parameters\n");
#if defined(CONFIG_BT_GATT_AUTO_DISCOVER_CCC)
	__ASSERT(params->ccc_handle ||
		 (params->end_handle && params->disc_params),
		 "invalid parameters\n");
#else
	__ASSERT(params->ccc_handle, "invalid parameters\n");
#endif

	if (conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	sub = gatt_sub_add(conn);
	if (!sub) {
		return -ENOMEM;
	}

	/* Lookup existing subscriptions */
	SYS_SLIST_FOR_EACH_CONTAINER(&sub->list, tmp, node) {
		/* Fail if entry already exists */
		if (tmp == params) {
			gatt_sub_remove(conn, sub, NULL, NULL);
			return -EALREADY;
		}

		/* Check if another subscription exists */
		if (tmp->value_handle == params->value_handle &&
		    tmp->value >= params->value) {
			has_subscription = true;
		}
	}

	/* Skip write if already subscribed */
	if (!has_subscription) {
		int err;

#if defined(CONFIG_BT_GATT_AUTO_DISCOVER_CCC)
		if (!params->ccc_handle) {
			err = gatt_ccc_discover(conn, params);
			if (err) {
				return err;
			}
		}
#endif
		err = gatt_write_ccc(conn, params->ccc_handle, params->value,
				     gatt_write_ccc_rsp, params);
		if (err) {
			gatt_sub_remove(conn, sub, NULL, NULL);
			return err;
		}
	}

	/*
	 * Add subscription before write complete as some implementation were
	 * reported to send notification before reply to CCC write.
	 */
	sys_slist_prepend(&sub->list, &params->node);

	return 0;
}

int bt_gatt_resubscribe(uint8_t id, const bt_addr_le_t *peer,
			     struct bt_gatt_subscribe_params *params)
{
	struct gatt_sub *sub;
	struct bt_gatt_subscribe_params *tmp;

	__ASSERT(params && params->notify,  "invalid parameters\n");
	__ASSERT(params->value, "invalid parameters\n");
	__ASSERT(params->ccc_handle, "invalid parameters\n");

	sub = gatt_sub_add_by_addr(id, peer);
	if (!sub) {
		return -ENOMEM;
	}

	/* Lookup existing subscriptions */
	SYS_SLIST_FOR_EACH_CONTAINER(&sub->list, tmp, node) {
		/* Fail if entry already exists */
		if (tmp == params) {
			gatt_sub_remove(NULL, sub, NULL, NULL);
			return -EALREADY;
		}
	}

	sys_slist_prepend(&sub->list, &params->node);
	return 0;
}

int bt_gatt_unsubscribe(struct bt_conn *conn,
			struct bt_gatt_subscribe_params *params)
{
	struct gatt_sub *sub;
	struct bt_gatt_subscribe_params *tmp, *next;
	bool has_subscription = false, found = false;
	sys_snode_t *prev = NULL;

	__ASSERT(conn, "invalid parameters\n");
	__ASSERT(params, "invalid parameters\n");

	if (conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	sub = gatt_sub_find(conn);
	if (!sub) {
		return -EINVAL;
	}

	/* Lookup existing subscriptions */
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&sub->list, tmp, next, node) {
		/* Remove subscription */
		if (params == tmp) {
			found = true;
			sys_slist_remove(&sub->list, prev, &tmp->node);
			/* Attempt to cancel if write is pending */
			if (atomic_test_bit(params->flags,
			    BT_GATT_SUBSCRIBE_FLAG_WRITE_PENDING)) {
				bt_gatt_cancel(conn, params);
			}
			continue;
		} else {
			prev = &tmp->node;
		}

		/* Check if there still remains any other subscription */
		if (tmp->value_handle == params->value_handle) {
			has_subscription = true;
		}
	}

	if (!found) {
		return -EINVAL;
	}

	if (has_subscription) {
		/* Notify with NULL data to complete unsubscribe */
		params->notify(conn, params, NULL, 0);
		return 0;
	}

	params->value = 0x0000;

	return gatt_write_ccc(conn, params->ccc_handle, params->value,
			      gatt_write_ccc_rsp, params);
}

void bt_gatt_cancel(struct bt_conn *conn, void *params)
{
	bt_att_req_cancel(conn, params);
}

static void add_subscriptions(struct bt_conn *conn)
{
	struct gatt_sub *sub;
	struct bt_gatt_subscribe_params *params;

	sub = gatt_sub_find(conn);
	if (!sub) {
		return;
	}

	/* Lookup existing subscriptions */
	SYS_SLIST_FOR_EACH_CONTAINER(&sub->list, params, node) {
		if (bt_addr_le_is_bonded(conn->id, &conn->le.dst) &&
		    !atomic_test_bit(params->flags,
				     BT_GATT_SUBSCRIBE_FLAG_NO_RESUB)) {
			/* Force write to CCC to workaround devices that don't
			 * track it properly.
			 */
			gatt_write_ccc(conn, params->ccc_handle, params->value,
				       gatt_write_ccc_rsp, params);
		}
	}
}
#endif /* CONFIG_BT_GATT_CLIENT */

#define CCC_STORE_MAX 48

static struct bt_gatt_ccc_cfg *ccc_find_cfg(struct _bt_gatt_ccc *ccc,
					    const bt_addr_le_t *addr,
					    uint8_t id)
{
	for (size_t i = 0; i < ARRAY_SIZE(ccc->cfg); i++) {
		if (id == ccc->cfg[i].id &&
		    !bt_addr_le_cmp(&ccc->cfg[i].peer, addr)) {
			return &ccc->cfg[i];
		}
	}

	return NULL;
}

struct addr_with_id {
	const bt_addr_le_t *addr;
	uint8_t id;
};

struct ccc_load {
	struct addr_with_id addr_with_id;
	struct ccc_store *entry;
	size_t count;
};

static void ccc_clear(struct _bt_gatt_ccc *ccc,
		      const bt_addr_le_t *addr,
		      uint8_t id)
{
	struct bt_gatt_ccc_cfg *cfg;

	cfg = ccc_find_cfg(ccc, addr, id);
	if (!cfg) {
		BT_DBG("Unable to clear CCC: cfg not found");
		return;
	}

	clear_ccc_cfg(cfg);
}

static uint8_t ccc_load(const struct bt_gatt_attr *attr, uint16_t handle,
			void *user_data)
{
	struct ccc_load *load = user_data;
	struct _bt_gatt_ccc *ccc;
	struct bt_gatt_ccc_cfg *cfg;

	/* Check if attribute is a CCC */
	if (attr->write != bt_gatt_attr_write_ccc) {
		return BT_GATT_ITER_CONTINUE;
	}

	ccc = attr->user_data;

	/* Clear if value was invalidated */
	if (!load->entry) {
		ccc_clear(ccc, load->addr_with_id.addr, load->addr_with_id.id);
		return BT_GATT_ITER_CONTINUE;
	} else if (!load->count) {
		return BT_GATT_ITER_STOP;
	}

	/* Skip if value is not for the given attribute */
	if (load->entry->handle != handle) {
		/* If attribute handle is bigger then it means
		 * the attribute no longer exists and cannot
		 * be restored.
		 */
		if (load->entry->handle < handle) {
			BT_DBG("Unable to restore CCC: handle 0x%04x cannot be"
			       " found",  load->entry->handle);
			goto next;
		}
		return BT_GATT_ITER_CONTINUE;
	}

	BT_DBG("Restoring CCC: handle 0x%04x value 0x%04x", load->entry->handle,
	       load->entry->value);

	cfg = ccc_find_cfg(ccc, load->addr_with_id.addr, load->addr_with_id.id);
	if (!cfg) {
		cfg = ccc_find_cfg(ccc, BT_ADDR_LE_ANY, 0);
		if (!cfg) {
			BT_DBG("Unable to restore CCC: no cfg left");
			goto next;
		}
		bt_addr_le_copy(&cfg->peer, load->addr_with_id.addr);
		cfg->id = load->addr_with_id.id;
	}

	cfg->value = load->entry->value;

next:
	load->entry++;
	load->count--;

	return load->count ? BT_GATT_ITER_CONTINUE : BT_GATT_ITER_STOP;
}

static int ccc_set(const char *name, size_t len_rd, settings_read_cb read_cb,
		   void *cb_arg)
{
	if (IS_ENABLED(CONFIG_BT_SETTINGS_CCC_LAZY_LOADING)) {
		/* Only load CCCs on demand */
		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		struct ccc_store ccc_store[CCC_STORE_MAX];
		struct ccc_load load;
		bt_addr_le_t addr;
		ssize_t len;
		int err;
		const char *next;

		settings_name_next(name, &next);

		if (!name) {
			BT_ERR("Insufficient number of arguments");
			return -EINVAL;
		} else if (!next) {
			load.addr_with_id.id = BT_ID_DEFAULT;
		} else {
			load.addr_with_id.id = strtol(next, NULL, 10);
		}

		err = bt_settings_decode_key(name, &addr);
		if (err) {
			BT_ERR("Unable to decode address %s", log_strdup(name));
			return -EINVAL;
		}

		load.addr_with_id.addr = &addr;

		if (len_rd) {
			len = read_cb(cb_arg, ccc_store, sizeof(ccc_store));

			if (len < 0) {
				BT_ERR("Failed to decode value (err %zd)", len);
				return len;
			}

			load.entry = ccc_store;
			load.count = len / sizeof(*ccc_store);

			for (size_t i = 0; i < load.count; i++) {
				BT_DBG("Read CCC: handle 0x%04x value 0x%04x",
				       ccc_store[i].handle, ccc_store[i].value);
			}
		} else {
			load.entry = NULL;
			load.count = 0;
		}

		bt_gatt_foreach_attr(0x0001, 0xffff, ccc_load, &load);

		BT_DBG("Restored CCC for id:%" PRIu8 " addr:%s",
		       load.addr_with_id.id,
		       bt_addr_le_str(load.addr_with_id.addr));
	}

	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(bt_ccc, "bt/ccc", NULL, ccc_set, NULL, NULL);

static int ccc_set_direct(const char *key, size_t len, settings_read_cb read_cb,
			  void *cb_arg, void *param)
{
	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		const char *name;

		BT_DBG("key: %s", log_strdup((const char *)param));

		/* Only "bt/ccc" settings should ever come here */
		if (!settings_name_steq((const char *)param, "bt/ccc", &name)) {
			BT_ERR("Invalid key");
			return -EINVAL;
		}

		return ccc_set(name, len, read_cb, cb_arg);
	}
	return 0;
}

void bt_gatt_connected(struct bt_conn *conn)
{
	struct conn_data data;

	BT_DBG("conn %p", conn);

	data.conn = conn;
	data.sec = BT_SECURITY_L1;

	/* Load CCC settings from backend if bonded */
	if (IS_ENABLED(CONFIG_BT_SETTINGS_CCC_LAZY_LOADING) &&
	    bt_addr_le_is_bonded(conn->id, &conn->le.dst)) {
		char key[BT_SETTINGS_KEY_MAX];

		if (conn->id) {
			char id_str[4];

			u8_to_dec(id_str, sizeof(id_str), conn->id);
			bt_settings_encode_key(key, sizeof(key), "ccc",
					       &conn->le.dst, id_str);
		} else {
			bt_settings_encode_key(key, sizeof(key), "ccc",
					       &conn->le.dst, NULL);
		}

		settings_load_subtree_direct(key, ccc_set_direct, (void *)key);
	}

	bt_gatt_foreach_attr(0x0001, 0xffff, update_ccc, &data);

	/* BLUETOOTH CORE SPECIFICATION Version 5.1 | Vol 3, Part C page 2192:
	 *
	 * 10.3.1.1 Handling of GATT indications and notifications
	 *
	 * A client “requests” a server to send indications and notifications
	 * by appropriately configuring the server via a Client Characteristic
	 * Configuration Descriptor. Since the configuration is persistent
	 * across a disconnection and reconnection, security requirements must
	 * be checked against the configuration upon a reconnection before
	 * sending indications or notifications. When a server reconnects to a
	 * client to send an indication or notification for which security is
	 * required, the server shall initiate or request encryption with the
	 * client prior to sending an indication or notification. If the client
	 * does not have an LTK indicating that the client has lost the bond,
	 * enabling encryption will fail.
	 */
	if (IS_ENABLED(CONFIG_BT_SMP) &&
	    bt_conn_get_security(conn) < data.sec) {
		bt_conn_set_security(conn, data.sec);
	}

#if defined(CONFIG_BT_GATT_CLIENT)
	add_subscriptions(conn);
#endif /* CONFIG_BT_GATT_CLIENT */
}

void bt_gatt_encrypt_change(struct bt_conn *conn)
{
	struct conn_data data;

	BT_DBG("conn %p", conn);

	data.conn = conn;
	data.sec = BT_SECURITY_L1;

	bt_gatt_foreach_attr(0x0001, 0xffff, update_ccc, &data);
}

bool bt_gatt_change_aware(struct bt_conn *conn, bool req)
{
#if defined(CONFIG_BT_GATT_CACHING)
	struct gatt_cf_cfg *cfg;

	cfg = find_cf_cfg(conn);
	if (!cfg || !CF_ROBUST_CACHING(cfg)) {
		return true;
	}

	if (atomic_test_bit(cfg->flags, CF_CHANGE_AWARE)) {
		return true;
	}

	/* BLUETOOTH CORE SPECIFICATION Version 5.1 | Vol 3, Part G page 2350:
	 * If a change-unaware client sends an ATT command, the server shall
	 * ignore it.
	 */
	if (!req) {
		return false;
	}

	/* BLUETOOTH CORE SPECIFICATION Version 5.1 | Vol 3, Part G page 2347:
	 * 2.5.2.1 Robust Caching
	 * A connected client becomes change-aware when...
	 * The server sends the client a response with the error code set to
	 * Database Out Of Sync and then the server receives another ATT
	 * request from the client.
	 */
	if (atomic_test_bit(cfg->flags, CF_OUT_OF_SYNC)) {
		atomic_clear_bit(cfg->flags, CF_OUT_OF_SYNC);
		atomic_set_bit(cfg->flags, CF_CHANGE_AWARE);
		BT_DBG("%s change-aware", bt_addr_le_str(&cfg->peer));
		return true;
	}

	atomic_set_bit(cfg->flags, CF_OUT_OF_SYNC);

	return false;
#else
	return true;
#endif
}

static int bt_gatt_store_cf(struct bt_conn *conn)
{
#if defined(CONFIG_BT_GATT_CACHING)
	struct gatt_cf_cfg *cfg;
	char key[BT_SETTINGS_KEY_MAX];
	char *str;
	size_t len;
	int err;

	cfg = find_cf_cfg(conn);
	if (!cfg) {
		/* No cfg found, just clear it */
		BT_DBG("No config for CF");
		str = NULL;
		len = 0;
	} else {
		str = (char *)cfg->data;
		len = sizeof(cfg->data);

		if (conn->id) {
			char id_str[4];

			u8_to_dec(id_str, sizeof(id_str), conn->id);
			bt_settings_encode_key(key, sizeof(key), "cf",
					       &conn->le.dst, id_str);
		}
	}

	if (!cfg || !conn->id) {
		bt_settings_encode_key(key, sizeof(key), "cf",
				       &conn->le.dst, NULL);
	}

	err = settings_save_one(key, str, len);
	if (err) {
		BT_ERR("Failed to store Client Features (err %d)", err);
		return err;
	}

	BT_DBG("Stored CF for %s (%s)", bt_addr_le_str(&conn->le.dst), log_strdup(key));
#endif /* CONFIG_BT_GATT_CACHING */
	return 0;

}

static struct gatt_cf_cfg *find_cf_cfg_by_addr(uint8_t id,
					       const bt_addr_le_t *addr)
{
	if (IS_ENABLED(CONFIG_BT_GATT_CACHING)) {
		int i;

		for (i = 0; i < ARRAY_SIZE(cf_cfg); i++) {
			if (id == cf_cfg[i].id &&
			    !bt_addr_le_cmp(addr, &cf_cfg[i].peer)) {
				return &cf_cfg[i];
			}
		}
	}

	return NULL;
}

#if defined(CONFIG_BT_SETTINGS)

struct ccc_save {
	struct addr_with_id addr_with_id;
	struct ccc_store store[CCC_STORE_MAX];
	size_t count;
};

static uint8_t ccc_save(const struct bt_gatt_attr *attr, uint16_t handle,
			void *user_data)
{
	struct ccc_save *save = user_data;
	struct _bt_gatt_ccc *ccc;
	struct bt_gatt_ccc_cfg *cfg;

	/* Check if attribute is a CCC */
	if (attr->write != bt_gatt_attr_write_ccc) {
		return BT_GATT_ITER_CONTINUE;
	}

	ccc = attr->user_data;

	/* Check if there is a cfg for the peer */
	cfg = ccc_find_cfg(ccc, save->addr_with_id.addr, save->addr_with_id.id);
	if (!cfg) {
		return BT_GATT_ITER_CONTINUE;
	}

	BT_DBG("Storing CCCs handle 0x%04x value 0x%04x", handle, cfg->value);

	save->store[save->count].handle = handle;
	save->store[save->count].value = cfg->value;
	save->count++;

	return BT_GATT_ITER_CONTINUE;
}

int bt_gatt_store_ccc(uint8_t id, const bt_addr_le_t *addr)
{
	struct ccc_save save;
	char key[BT_SETTINGS_KEY_MAX];
	size_t len;
	char *str;
	int err;

	save.addr_with_id.addr = addr;
	save.addr_with_id.id = id;
	save.count = 0;

	bt_gatt_foreach_attr(0x0001, 0xffff, ccc_save, &save);

	if (id) {
		char id_str[4];

		u8_to_dec(id_str, sizeof(id_str), id);
		bt_settings_encode_key(key, sizeof(key), "ccc", addr, id_str);
	} else {
		bt_settings_encode_key(key, sizeof(key), "ccc", addr, NULL);
	}

	if (save.count) {
		str = (char *)save.store;
		len = save.count * sizeof(*save.store);
	} else {
		/* No entries to encode, just clear */
		str = NULL;
		len = 0;
	}

	err = settings_save_one(key, str, len);
	if (err) {
		BT_ERR("Failed to store CCCs (err %d)", err);
		return err;
	}

	BT_DBG("Stored CCCs for %s (%s)", bt_addr_le_str(addr),
	       log_strdup(key));
	if (len) {
		for (size_t i = 0; i < save.count; i++) {
			BT_DBG("  CCC: handle 0x%04x value 0x%04x",
			       save.store[i].handle, save.store[i].value);
		}
	} else {
		BT_DBG("  CCC: NULL");
	}

	return 0;
}

#if defined(CONFIG_BT_GATT_SERVICE_CHANGED)
static int sc_set(const char *name, size_t len_rd, settings_read_cb read_cb,
		  void *cb_arg)
{
	struct gatt_sc_cfg *cfg;
	uint8_t id;
	bt_addr_le_t addr;
	ssize_t len;
	int err;
	const char *next;

	if (!name) {
		BT_ERR("Insufficient number of arguments");
		return -EINVAL;
	}

	err = bt_settings_decode_key(name, &addr);
	if (err) {
		BT_ERR("Unable to decode address %s", log_strdup(name));
		return -EINVAL;
	}

	settings_name_next(name, &next);

	if (!next) {
		id = BT_ID_DEFAULT;
	} else {
		id = strtol(next, NULL, 10);
	}

	cfg = find_sc_cfg(id, &addr);
	if (!cfg && len_rd) {
		/* Find and initialize a free sc_cfg entry */
		cfg = find_sc_cfg(BT_ID_DEFAULT, BT_ADDR_LE_ANY);
		if (!cfg) {
			BT_ERR("Unable to restore SC: no cfg left");
			return -ENOMEM;
		}

		cfg->id = id;
		bt_addr_le_copy(&cfg->peer, &addr);
	}

	if (len_rd) {
		len = read_cb(cb_arg, &cfg->data, sizeof(cfg->data));
		if (len < 0) {
			BT_ERR("Failed to decode value (err %zd)", len);
			return len;
		}

		BT_DBG("Read SC: len %zd", len);

		BT_DBG("Restored SC for %s", bt_addr_le_str(&addr));
	} else if (cfg) {
		/* Clear configuration */
		memset(cfg, 0, sizeof(*cfg));

		BT_DBG("Removed SC for %s", bt_addr_le_str(&addr));
	}

	return 0;
}

static int sc_commit(void)
{
	atomic_clear_bit(gatt_sc.flags, SC_INDICATE_PENDING);

	if (atomic_test_bit(gatt_sc.flags, SC_RANGE_CHANGED)) {
		/* Schedule SC indication since the range has changed */
		k_delayed_work_submit(&gatt_sc.work, SC_TIMEOUT);
	}

	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(bt_sc, "bt/sc", NULL, sc_set, sc_commit, NULL);
#endif /* CONFIG_BT_GATT_SERVICE_CHANGED */

#if defined(CONFIG_BT_GATT_CACHING)
static int cf_set(const char *name, size_t len_rd, settings_read_cb read_cb,
		  void *cb_arg)
{
	struct gatt_cf_cfg *cfg;
	bt_addr_le_t addr;
	const char *next;
	ssize_t len;
	int err;
	uint8_t id;

	if (!name) {
		BT_ERR("Insufficient number of arguments");
		return -EINVAL;
	}

	err = bt_settings_decode_key(name, &addr);
	if (err) {
		BT_ERR("Unable to decode address %s", log_strdup(name));
		return -EINVAL;
	}

	settings_name_next(name, &next);

	if (!next) {
		id = BT_ID_DEFAULT;
	} else {
		id = strtol(next, NULL, 10);
	}

	cfg = find_cf_cfg_by_addr(id, &addr);
	if (!cfg) {
		cfg = find_cf_cfg(NULL);
		if (!cfg) {
			BT_ERR("Unable to restore CF: no cfg left");
			return -ENOMEM;
		}

		cfg->id = id;
		bt_addr_le_copy(&cfg->peer, &addr);
	}

	if (len_rd) {
		len = read_cb(cb_arg, cfg->data, sizeof(cfg->data));
		if (len < 0) {
			BT_ERR("Failed to decode value (err %zd)", len);
			return len;
		}

		BT_DBG("Read CF: len %zd", len);
	} else {
		clear_cf_cfg(cfg);
	}

	BT_DBG("Restored CF for %s", bt_addr_le_str(&addr));

	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(bt_cf, "bt/cf", NULL, cf_set, NULL, NULL);

static uint8_t stored_hash[16];

static int db_hash_set(const char *name, size_t len_rd,
		       settings_read_cb read_cb, void *cb_arg)
{
	ssize_t len;

	len = read_cb(cb_arg, stored_hash, sizeof(stored_hash));
	if (len < 0) {
		BT_ERR("Failed to decode value (err %zd)", len);
		return len;
	}

	BT_HEXDUMP_DBG(stored_hash, sizeof(stored_hash), "Stored Hash: ");

	return 0;
}

static int db_hash_commit(void)
{
	int err;

	/* Stop work and generate the hash */
	err = k_delayed_work_cancel(&db_hash_work);
	if (!err) {
		db_hash_gen(false);
	}

	/* Check if hash matches then skip SC update */
	if (!memcmp(stored_hash, db_hash, sizeof(stored_hash))) {
		BT_DBG("Database Hash matches");
		k_delayed_work_cancel(&gatt_sc.work);
		atomic_clear_bit(gatt_sc.flags, SC_RANGE_CHANGED);
		return 0;
	}

	BT_HEXDUMP_DBG(db_hash, sizeof(db_hash), "New Hash: ");

	/**
	 * GATT database has been modified since last boot, likely due to
	 * a firmware update or a dynamic service that was not re-registered on
	 * boot. Indicate Service Changed to all bonded devices for the full
	 * database range to invalidate client-side cache and force discovery on
	 * reconnect.
	 */
	sc_indicate(0x0001, 0xffff);

	/* Hash did not match overwrite with current hash */
	db_hash_store();

	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(bt_hash, "bt/hash", NULL, db_hash_set,
			       db_hash_commit, NULL);
#endif /*CONFIG_BT_GATT_CACHING */
#endif /* CONFIG_BT_SETTINGS */

static uint8_t remove_peer_from_attr(const struct bt_gatt_attr *attr,
				     uint16_t handle, void *user_data)
{
	const struct addr_with_id *addr_with_id = user_data;
	struct _bt_gatt_ccc *ccc;
	struct bt_gatt_ccc_cfg *cfg;

	/* Check if attribute is a CCC */
	if (attr->write != bt_gatt_attr_write_ccc) {
		return BT_GATT_ITER_CONTINUE;
	}

	ccc = attr->user_data;

	/* Check if there is a cfg for the peer */
	cfg = ccc_find_cfg(ccc, addr_with_id->addr, addr_with_id->id);
	if (cfg) {
		memset(cfg, 0, sizeof(*cfg));
	}

	return BT_GATT_ITER_CONTINUE;
}

static int bt_gatt_clear_ccc(uint8_t id, const bt_addr_le_t *addr)
{
	struct addr_with_id addr_with_id = {
		.addr = addr,
		.id = id,
	};

	bt_gatt_foreach_attr(0x0001, 0xffff, remove_peer_from_attr,
			     &addr_with_id);

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		char key[BT_SETTINGS_KEY_MAX];

		if (id) {
			char id_str[4];

			u8_to_dec(id_str, sizeof(id_str), id);
			bt_settings_encode_key(key, sizeof(key), "ccc",
					       addr, id_str);
		} else {
			bt_settings_encode_key(key, sizeof(key), "ccc",
					       addr, NULL);
		}

		return settings_delete(key);
	}

	return 0;
}

static int bt_gatt_clear_cf(uint8_t id, const bt_addr_le_t *addr)
{
	struct gatt_cf_cfg *cfg;

	cfg = find_cf_cfg_by_addr(id, addr);
	if (cfg) {
		clear_cf_cfg(cfg);
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		char key[BT_SETTINGS_KEY_MAX];

		if (id) {
			char id_str[4];

			u8_to_dec(id_str, sizeof(id_str), id);
			bt_settings_encode_key(key, sizeof(key), "cf",
					       addr, id_str);
		} else {
			bt_settings_encode_key(key, sizeof(key), "cf",
					       addr, NULL);
		}

		return settings_delete(key);
	}

	return 0;

}


static struct gatt_sub *find_gatt_sub(uint8_t id, const bt_addr_le_t *addr)
{
	for (int i = 0; i < ARRAY_SIZE(subscriptions); i++) {
		struct gatt_sub *sub = &subscriptions[i];

		if (id == sub->id &&
		    !bt_addr_le_cmp(addr, &sub->peer)) {
			return sub;
		}
	}

	return NULL;
}

static void bt_gatt_clear_subscriptions(uint8_t id, const bt_addr_le_t *addr)
{
	struct gatt_sub *sub;
	struct bt_gatt_subscribe_params *params, *tmp;
	sys_snode_t *prev = NULL;

	sub = find_gatt_sub(id, addr);
	if (!sub) {
		return;
	}

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&sub->list, params, tmp,
					  node) {
		params->value = 0U;
		gatt_sub_remove(NULL, sub, prev, params);
	}
}

int bt_gatt_clear(uint8_t id, const bt_addr_le_t *addr)
{
	int err;

	err = bt_gatt_clear_ccc(id, addr);
	if (err < 0) {
		return err;
	}

	if (IS_ENABLED(CONFIG_BT_GATT_SERVICE_CHANGED)) {
		err = bt_gatt_clear_sc(id, addr);
		if (err < 0) {
			return err;
		}
	}

	if (IS_ENABLED(CONFIG_BT_GATT_CACHING)) {
		err = bt_gatt_clear_cf(id, addr);
		if (err < 0) {
			return err;
		}
	}

	if (IS_ENABLED(CONFIG_BT_GATT_CLIENT)) {
		bt_gatt_clear_subscriptions(id, addr);
	}

	return 0;
}

void bt_gatt_disconnected(struct bt_conn *conn)
{
	BT_DBG("conn %p", conn);
	bt_gatt_foreach_attr(0x0001, 0xffff, disconnected_cb, conn);

#if defined(CONFIG_BT_SETTINGS_CCC_STORE_ON_WRITE)
	gatt_ccc_conn_unqueue(conn);

	if (gatt_ccc_conn_queue_is_empty()) {
		k_delayed_work_cancel(&gatt_ccc_store.work);
	}
#endif

	if (IS_ENABLED(CONFIG_BT_SETTINGS) &&
	    bt_addr_le_is_bonded(conn->id, &conn->le.dst)) {
		bt_gatt_store_ccc(conn->id, &conn->le.dst);
		bt_gatt_store_cf(conn);
	}

	/* Make sure to clear the CCC entry when using lazy loading */
	if (IS_ENABLED(CONFIG_BT_SETTINGS_CCC_LAZY_LOADING) &&
	    bt_addr_le_is_bonded(conn->id, &conn->le.dst)) {
		struct addr_with_id addr_with_id = {
			.addr = &conn->le.dst,
			.id = conn->id,
		};
		bt_gatt_foreach_attr(0x0001, 0xffff,
				     remove_peer_from_attr,
				     &addr_with_id);
	}

#if defined(CONFIG_BT_GATT_CLIENT)
	remove_subscriptions(conn);
#endif /* CONFIG_BT_GATT_CLIENT */

#if defined(CONFIG_BT_GATT_CACHING)
	remove_cf_cfg(conn);
#endif
}
