/* gatt.c - Generic Attribute Profile handling */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sys/types.h"
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys_clock.h>
#include <zephyr/toolchain.h>

#if defined(CONFIG_BT_GATT_CACHING)
#include <psa/crypto.h>
#include <psa/crypto_struct.h>
#include <psa/crypto_types.h>
#include <psa/crypto_values.h>
#endif /* CONFIG_BT_GATT_CACHING */

#include "att_internal.h"
#include "conn_internal.h"
#include "common/bt_str.h"
#include "gatt_internal.h"
#include "hci_core.h"
#include "keys.h"
#include "long_wq.h"
#include "l2cap_internal.h"
#include "settings.h"
#include "smp.h"

#define LOG_LEVEL CONFIG_BT_GATT_LOG_LEVEL
LOG_MODULE_REGISTER(bt_gatt);

#define SC_TIMEOUT	K_MSEC(10)
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

/**
 * Entry x is free for reuse whenever (subscriptions[x].peer == BT_ADDR_LE_ANY).
 * Invariant: (sys_slist_is_empty(subscriptions[x].list))
 *              <=> (subscriptions[x].peer == BT_ADDR_LE_ANY).
 */
static struct gatt_sub subscriptions[SUB_MAX];
static sys_slist_t callback_list = SYS_SLIST_STATIC_INIT(&callback_list);

#if defined(CONFIG_BT_GATT_DYNAMIC_DB)
static sys_slist_t db;
#endif /* CONFIG_BT_GATT_DYNAMIC_DB */

enum gatt_global_flags {
	GATT_INITIALIZED,
	GATT_SERVICE_INITIALIZED,

	GATT_NUM_FLAGS,
};

static ATOMIC_DEFINE(gatt_flags, GATT_NUM_FLAGS);

static ssize_t read_name(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	const char *name = bt_get_name();

	return bt_gatt_attr_read(conn, attr, buf, len, offset, name,
				 strlen(name));
}

#if defined(CONFIG_BT_DEVICE_NAME_GATT_WRITABLE)

static ssize_t write_name(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
			  uint16_t len, uint16_t offset, uint8_t flags)
{
	/* adding one to fit the terminating null character */
	char value[CONFIG_BT_DEVICE_NAME_MAX + 1] = {};

	if (offset >= CONFIG_BT_DEVICE_NAME_MAX) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (offset + len > CONFIG_BT_DEVICE_NAME_MAX) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(value, buf, len);

	value[len] = '\0';

	bt_set_name(value);

	return len;
}

#endif /* CONFIG_BT_DEVICE_NAME_GATT_WRITABLE */

static ssize_t read_appearance(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	uint16_t appearance = sys_cpu_to_le16(bt_get_appearance());

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &appearance,
				 sizeof(appearance));
}

#if defined(CONFIG_BT_DEVICE_APPEARANCE_GATT_WRITABLE)
static ssize_t write_appearance(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, uint16_t len, uint16_t offset,
			 uint8_t flags)
{
	uint16_t appearance_le = sys_cpu_to_le16(bt_get_appearance());
	char * const appearance_le_bytes = (char *)&appearance_le;
	uint16_t appearance;
	int err;

	if (offset >= sizeof(appearance_le)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if ((offset + len) > sizeof(appearance_le)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(&appearance_le_bytes[offset], buf, len);
	appearance = sys_le16_to_cpu(appearance_le);

	err = bt_set_appearance(appearance);

	if (err) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	return len;
}
#endif /* CONFIG_BT_DEVICE_APPEARANCE_GATT_WRITABLE */

#if defined(CONFIG_BT_DEVICE_APPEARANCE_GATT_WRITABLE)
	#define GAP_APPEARANCE_PROPS (BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE)
#if defined(CONFIG_DEVICE_APPEARANCE_GATT_WRITABLE_AUTHEN)
	#define GAP_APPEARANCE_PERMS (BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_AUTHEN)
#elif defined(CONFIG_BT_DEVICE_APPEARANCE_GATT_WRITABLE_ENCRYPT)
	#define GAP_APPEARANCE_PERMS (BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT)
#else
	#define GAP_APPEARANCE_PERMS (BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)
#endif
	#define GAP_APPEARANCE_WRITE_HANDLER write_appearance
#else
	#define GAP_APPEARANCE_PROPS BT_GATT_CHRC_READ
	#define GAP_APPEARANCE_PERMS BT_GATT_PERM_READ
	#define GAP_APPEARANCE_WRITE_HANDLER NULL
#endif

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
BUILD_ASSERT((CONFIG_BT_PERIPHERAL_PREF_TIMEOUT * 4U) >
	     ((1U + CONFIG_BT_PERIPHERAL_PREF_LATENCY) *
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
	ppcp.latency = sys_cpu_to_le16(CONFIG_BT_PERIPHERAL_PREF_LATENCY);
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
			       BT_GATT_PERM_READ |
#if defined(CONFIG_DEVICE_NAME_GATT_WRITABLE_AUTHEN)
			       BT_GATT_PERM_WRITE_AUTHEN,
#elif defined(CONFIG_DEVICE_NAME_GATT_WRITABLE_ENCRYPT)
			       BT_GATT_PERM_WRITE_ENCRYPT,
#else
			       BT_GATT_PERM_WRITE,
#endif
			       read_name, write_name, bt_dev.name),
#else
	BT_GATT_CHARACTERISTIC(BT_UUID_GAP_DEVICE_NAME, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_name, NULL, NULL),
#endif /* CONFIG_BT_DEVICE_NAME_GATT_WRITABLE */
	BT_GATT_CHARACTERISTIC(BT_UUID_GAP_APPEARANCE, GAP_APPEARANCE_PROPS,
			       GAP_APPEARANCE_PERMS, read_appearance,
			       GAP_APPEARANCE_WRITE_HANDLER, NULL),
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

#if defined(CONFIG_BT_GATT_SERVICE_CHANGED)
#define SC_CFG_MAX (CONFIG_BT_MAX_PAIRED + CONFIG_BT_MAX_CONN)
#else
#define SC_CFG_MAX 0
#endif
static struct gatt_sc_cfg sc_cfg[SC_CFG_MAX];
BUILD_ASSERT(sizeof(struct sc_data) == sizeof(sc_cfg[0].data));

enum {
	SC_RANGE_CHANGED,    /* SC range changed */
	SC_INDICATE_PENDING, /* SC indicate pending */
	SC_LOAD,	     /* SC has been loaded from settings */

	DB_HASH_VALID,       /* Database hash needs to be calculated */
	DB_HASH_LOAD,        /* Database hash loaded from settings. */
	DB_HASH_LOAD_PROC,   /* DB hash loaded from settings has been processed. */

	/* Total number of flags - must be at the end of the enum */
	SC_NUM_FLAGS,
};

#if defined(CONFIG_BT_GATT_SERVICE_CHANGED)
static struct gatt_sc {
	struct bt_gatt_indicate_params params;
	uint16_t start;
	uint16_t end;
	struct k_work_delayable work;

	ATOMIC_DEFINE(flags, SC_NUM_FLAGS);
} gatt_sc;
#endif /* defined(CONFIG_BT_GATT_SERVICE_CHANGED) */

#if defined(CONFIG_BT_GATT_CACHING)
static struct db_hash {
	uint8_t hash[16];
#if defined(CONFIG_BT_SETTINGS)
	 uint8_t stored_hash[16];
#endif
	struct k_work_delayable work;
	struct k_work_sync sync;
} db_hash;
#endif

static struct gatt_sc_cfg *find_sc_cfg(uint8_t id, const bt_addr_le_t *addr)
{
	LOG_DBG("id: %u, addr: %s", id, bt_addr_le_str(addr));

	for (size_t i = 0; i < ARRAY_SIZE(sc_cfg); i++) {
		if (id == sc_cfg[i].id &&
		    bt_addr_le_eq(&sc_cfg[i].peer, addr)) {
			return &sc_cfg[i];
		}
	}

	return NULL;
}

static void sc_store(struct gatt_sc_cfg *cfg)
{
	int err;

	err = bt_settings_store_sc(cfg->id, &cfg->peer, &cfg->data, sizeof(cfg->data));
	if (err) {
		LOG_ERR("failed to store SC (err %d)", err);
		return;
	}

	LOG_DBG("stored SC for %s (0x%04x-0x%04x)", bt_addr_le_str(&cfg->peer), cfg->data.start,
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
		int err;

		err = bt_settings_delete_sc(cfg->id, &cfg->peer);
		if (err) {
			LOG_ERR("failed to delete SC (err %d)", err);
		} else {
			LOG_DBG("deleted SC for %s", bt_addr_le_str(&cfg->peer));
		}
	}

	clear_sc_cfg(cfg);

	return 0;
}

static void sc_clear(struct bt_conn *conn)
{
	if (bt_le_bond_exists(conn->id, &conn->le.dst)) {
		int err;

		err = bt_gatt_clear_sc(conn->id, &conn->le.dst);
		if (err) {
			LOG_ERR("Failed to clear SC %d", err);
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
	LOG_DBG("peer %s", bt_addr_le_str(&cfg->peer));

	memset(&cfg->data, 0, sizeof(cfg->data));

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		sc_store(cfg);
	}
}

static bool update_range(uint16_t *start, uint16_t *end, uint16_t new_start,
			 uint16_t new_end)
{
	LOG_DBG("start 0x%04x end 0x%04x new_start 0x%04x new_end 0x%04x", *start, *end, new_start,
		new_end);

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

	LOG_DBG("peer %s start 0x%04x end 0x%04x", bt_addr_le_str(peer), start, end);

	cfg = find_sc_cfg(id, peer);
	if (!cfg) {
		/* Find and initialize a free sc_cfg entry */
		cfg = find_sc_cfg(BT_ID_DEFAULT, BT_ADDR_LE_ANY);
		if (!cfg) {
			LOG_ERR("unable to save SC: no cfg left");
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
	if (IS_ENABLED(CONFIG_BT_SETTINGS) && modified && bt_le_bond_exists(cfg->id, &cfg->peer)) {
		sc_store(cfg);
	}
}

static ssize_t sc_ccc_cfg_write(struct bt_conn *conn,
				const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("value 0x%04x", value);

	if (value == BT_GATT_CCC_INDICATE) {
		/* Create a new SC configuration entry if subscribed */
		sc_save(conn->id, &conn->le.dst, 0, 0);
	} else {
		sc_clear(conn);
	}

	return sizeof(value);
}

static struct bt_gatt_ccc_managed_user_data sc_ccc =
	BT_GATT_CCC_MANAGED_USER_DATA_INIT(NULL, sc_ccc_cfg_write, NULL);

/* Do not shuffle the values in this enum, they are used as bit offsets when
 * saving the CF flags to NVS (i.e. NVS persists between FW upgrades).
 */
enum {
	CF_CHANGE_AWARE,	/* Client is changed aware */
	CF_DB_HASH_READ,	/* The client has read the database hash */

	/* Total number of flags - must be at the end of the enum */
	CF_NUM_FLAGS,
};

#define CF_BIT_ROBUST_CACHING	0
#define CF_BIT_EATT		1
#define CF_BIT_NOTIFY_MULTI	2
#define CF_BIT_LAST		CF_BIT_NOTIFY_MULTI

#define CF_NUM_BITS		(CF_BIT_LAST + 1)
#define CF_NUM_BYTES		((CF_BIT_LAST / 8) + 1)
#define CF_FLAGS_STORE_LEN	1

#define CF_ROBUST_CACHING(_cfg) (_cfg->data[0] & BIT(CF_BIT_ROBUST_CACHING))
#define CF_EATT(_cfg) (_cfg->data[0] & BIT(CF_BIT_EATT))
#define CF_NOTIFY_MULTI(_cfg) (_cfg->data[0] & BIT(CF_BIT_NOTIFY_MULTI))

struct gatt_cf_cfg {
	uint8_t			id;
	bt_addr_le_t		peer;
	uint8_t			data[CF_NUM_BYTES];
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

enum delayed_store_flags {
	DELAYED_STORE_CCC,
	DELAYED_STORE_CF,
	DELAYED_STORE_NUM_FLAGS
};

#if defined(CONFIG_BT_SETTINGS_DELAYED_STORE)
static void gatt_delayed_store_enqueue(uint8_t id, const bt_addr_le_t *peer_addr,
				       enum delayed_store_flags flag);
#endif

#if defined(CONFIG_BT_GATT_CACHING)
static bool set_change_aware_no_store(struct gatt_cf_cfg *cfg, bool aware)
{
	bool changed;

	if (aware) {
		changed = !atomic_test_and_set_bit(cfg->flags, CF_CHANGE_AWARE);
	} else {
		changed = atomic_test_and_clear_bit(cfg->flags, CF_CHANGE_AWARE);
	}

	LOG_DBG("peer is now change-%saware", aware ? "" : "un");

	return changed;
}

static void set_change_aware(struct gatt_cf_cfg *cfg, bool aware)
{
	bool changed = set_change_aware_no_store(cfg, aware);

#if defined(CONFIG_BT_SETTINGS_DELAYED_STORE)
	if (changed) {
		gatt_delayed_store_enqueue(cfg->id, &cfg->peer, DELAYED_STORE_CF);
	}
#else
	(void)changed;
#endif
}

static int bt_gatt_store_cf(uint8_t id, const bt_addr_le_t *peer);

static void set_all_change_unaware(void)
{
#if defined(CONFIG_BT_SETTINGS)
	/* Mark all bonded peers as change-unaware.
	 * - Can be called when not in a connection with said peers
	 * - Doesn't have any effect when no bonds are in memory. This is the
	 *   case when the device has just booted and `settings_load` hasn't yet
	 *   been called.
	 * - Expensive to call, as it will write the new status to settings
	 *   right away.
	 */
	for (size_t i = 0; i < ARRAY_SIZE(cf_cfg); i++) {
		struct gatt_cf_cfg *cfg = &cf_cfg[i];

		if (!bt_addr_le_eq(&cfg->peer, BT_ADDR_LE_ANY)) {
			set_change_aware_no_store(cfg, false);
			bt_gatt_store_cf(cfg->id, &cfg->peer);
		}
	}
#endif	/* CONFIG_BT_SETTINGS */
}

static struct gatt_cf_cfg *find_cf_cfg(struct bt_conn *conn)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(cf_cfg); i++) {
		struct gatt_cf_cfg *cfg = &cf_cfg[i];

		if (!conn) {
			if (bt_addr_le_eq(&cfg->peer, BT_ADDR_LE_ANY)) {
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

	/* Validate the bits */
	for (i = 0U; i <= CF_BIT_LAST && (i / 8) < len; i++) {
		if ((cfg->data[i / 8] & BIT(i % 8)) &&
		    !(value[i / 8] & BIT(i % 8))) {
			/* A client shall never clear a bit it has set */
			return false;
		}
	}

	/* Set the bits for each octet */
	for (i = 0U; i < len && i < CF_NUM_BYTES; i++) {
		if (i == (CF_NUM_BYTES - 1)) {
			cfg->data[i] |= value[i] & BIT_MASK(CF_NUM_BITS % BITS_PER_BYTE);
		} else {
			cfg->data[i] |= value[i];
		}

		LOG_DBG("byte %u: data 0x%02x value 0x%02x", i, cfg->data[i], value[i]);
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
		LOG_WRN("No space to store Client Supported Features");
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	LOG_DBG("handle 0x%04x len %u", attr->handle, len);

	if (!cf_set_value(cfg, value, len)) {
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	bt_addr_le_copy(&cfg->peer, &conn->le.dst);
	cfg->id = conn->id;
	set_change_aware(cfg, true);

	return len;
}

struct gen_hash_state {
	psa_mac_operation_t operation;
	psa_key_id_t key;
	int err;
};

static int db_hash_setup(struct gen_hash_state *state, uint8_t *key)
{
	psa_key_attributes_t key_attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t ret;

	psa_set_key_type(&key_attr, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&key_attr, 128);
	psa_set_key_usage_flags(&key_attr, PSA_KEY_USAGE_SIGN_MESSAGE);
	psa_set_key_algorithm(&key_attr, PSA_ALG_CMAC);

	ret = psa_import_key(&key_attr, key, 16, &(state->key));
	if (ret != PSA_SUCCESS) {
		LOG_ERR("Unable to import the key for AES CMAC %d", ret);
		return -EIO;
	}
	memset(&state->operation, 0, sizeof(state->operation));

	ret = psa_mac_sign_setup(&(state->operation), state->key, PSA_ALG_CMAC);
	if (ret != PSA_SUCCESS) {
		LOG_ERR("CMAC operation init failed %d", ret);
		return -EIO;
	}
	return 0;
}

static int db_hash_update(struct gen_hash_state *state, uint8_t *data, size_t len)
{
	psa_status_t ret = psa_mac_update(&(state->operation), data, len);

	if (ret != PSA_SUCCESS) {
		LOG_ERR("CMAC update failed %d", ret);
		return -EIO;
	}
	return 0;
}

static int db_hash_finish(struct gen_hash_state *state)
{
	size_t mac_length;
	psa_status_t ret = psa_mac_sign_finish(&(state->operation), db_hash.hash, 16, &mac_length);

	psa_destroy_key(state->key);

	if (ret != PSA_SUCCESS) {
		LOG_ERR("CMAC finish failed %d", ret);
		return -EIO;
	}
	return 0;
}

union hash_attr_value {
	/* Bluetooth Core Specification Version 5.3 | Vol 3, Part G
	 * Table 3.1: Service declaration
	 */
	union {
		uint16_t uuid16;
		uint8_t  uuid128[BT_UUID_SIZE_128];
	} __packed service;
	/* Bluetooth Core Specification Version 5.3 | Vol 3, Part G
	 * Table 3.2: Include declaration
	 */
	struct {
		uint16_t attribute_handle;
		uint16_t end_group_handle;
		uint16_t uuid16;
	} __packed inc;
	/* Bluetooth Core Specification Version 5.3 | Vol 3, Part G
	 * Table 3.3: Characteristic declaration
	 */
	struct {
		uint8_t properties;
		uint16_t value_handle;
		union {
			uint16_t uuid16;
			uint8_t  uuid128[BT_UUID_SIZE_128];
		} __packed;
	} __packed chrc;
	/* Bluetooth Core Specification Version 5.3 | Vol 3, Part G
	 * Table 3.5: Characteristic Properties bit field
	 */
	struct {
		uint16_t properties;
	} __packed cep;
} __packed;

static uint8_t gen_hash_m(const struct bt_gatt_attr *attr, uint16_t handle,
			  void *user_data)
{
	struct gen_hash_state *state = user_data;
	struct bt_uuid_16 *u16;
	uint8_t data[sizeof(union hash_attr_value)];
	ssize_t len;
	uint16_t value;

	if (attr->uuid->type != BT_UUID_TYPE_16) {
		return BT_GATT_ITER_CONTINUE;
	}

	u16 = (struct bt_uuid_16 *)attr->uuid;

	switch (u16->val) {
	/* Attributes to hash: handle + UUID + value */
	case BT_UUID_GATT_PRIMARY_VAL:
	case BT_UUID_GATT_SECONDARY_VAL:
	case BT_UUID_GATT_INCLUDE_VAL:
	case BT_UUID_GATT_CHRC_VAL:
	case BT_UUID_GATT_CEP_VAL:
		value = sys_cpu_to_le16(handle);
		if (db_hash_update(state, (uint8_t *)&value,
				   sizeof(handle)) != 0) {
			state->err = -EINVAL;
			return BT_GATT_ITER_STOP;
		}

		value = sys_cpu_to_le16(u16->val);
		if (db_hash_update(state, (uint8_t *)&value,
				   sizeof(u16->val)) != 0) {
			state->err = -EINVAL;
			return BT_GATT_ITER_STOP;
		}

		len = attr->read(NULL, attr, data, sizeof(data), 0);
		if (len < 0) {
			state->err = len;
			return BT_GATT_ITER_STOP;
		}

		if (db_hash_update(state, data, len) != 0) {
			state->err = -EINVAL;
			return BT_GATT_ITER_STOP;
		}

		break;
	/* Attributes to hash: handle + UUID */
	case BT_UUID_GATT_CUD_VAL:
	case BT_UUID_GATT_CCC_VAL:
	case BT_UUID_GATT_SCC_VAL:
	case BT_UUID_GATT_CPF_VAL:
	case BT_UUID_GATT_CAF_VAL:
		value = sys_cpu_to_le16(handle);
		if (db_hash_update(state, (uint8_t *)&value,
				   sizeof(handle)) != 0) {
			state->err = -EINVAL;
			return BT_GATT_ITER_STOP;
		}

		value = sys_cpu_to_le16(u16->val);
		if (db_hash_update(state, (uint8_t *)&value,
				   sizeof(u16->val)) != 0) {
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
#if defined(CONFIG_BT_SETTINGS)
	int err;

	err = bt_settings_store_hash(&db_hash.hash, sizeof(db_hash.hash));
	if (err) {
		LOG_ERR("Failed to save Database Hash (err %d)", err);
	}

	LOG_DBG("Database Hash stored");
#endif	/* CONFIG_BT_SETTINGS */
}

static void db_hash_gen(void)
{
	uint8_t key[16] = {};
	struct gen_hash_state state;

	if (db_hash_setup(&state, key) != 0) {
		return;
	}

	bt_gatt_foreach_attr(0x0001, 0xffff, gen_hash_m, &state);

	if (db_hash_finish(&state) != 0) {
		return;
	}

	/**
	 * Core 5.1 does not state the endianness of the hash.
	 * However Vol 3, Part F, 3.3.1 says that multi-octet Characteristic
	 * Values shall be LE unless otherwise defined. PTS expects hash to be
	 * in little endianness as well. bt_smp_aes_cmac calculates the hash in
	 * big endianness so we have to swap.
	 */
	sys_mem_swap(db_hash.hash, sizeof(db_hash.hash));

	LOG_HEXDUMP_DBG(db_hash.hash, sizeof(db_hash.hash), "Hash: ");

	atomic_set_bit(gatt_sc.flags, DB_HASH_VALID);
}

static void sc_indicate(uint16_t start, uint16_t end);

static void do_db_hash(void)
{
	bool new_hash = !atomic_test_bit(gatt_sc.flags, DB_HASH_VALID);

	if (new_hash) {
		db_hash_gen();
	}

#if defined(CONFIG_BT_SETTINGS)
	bool hash_loaded_from_settings =
		atomic_test_bit(gatt_sc.flags, DB_HASH_LOAD);
	bool already_processed =
		atomic_test_bit(gatt_sc.flags, DB_HASH_LOAD_PROC);

	if (!hash_loaded_from_settings) {
		/* we want to generate the hash, but not overwrite the hash
		 * stored in settings, that we haven't yet loaded.
		 */
		return;
	}

	if (already_processed) {
		/* hash has been loaded from settings and we have already
		 * executed the special case below once. we can now safely save
		 * the calculated hash to settings (if it has changed).
		 */
		if (new_hash) {
			set_all_change_unaware();
			db_hash_store();
		}
	} else {
		/* this is only supposed to run once, on bootup, after the hash
		 * has been loaded from settings.
		 */
		atomic_set_bit(gatt_sc.flags, DB_HASH_LOAD_PROC);

		/* Check if hash matches then skip SC update */
		if (!memcmp(db_hash.stored_hash, db_hash.hash,
			    sizeof(db_hash.stored_hash))) {
			LOG_DBG("Database Hash matches");
			k_work_cancel_delayable(&gatt_sc.work);
			atomic_clear_bit(gatt_sc.flags, SC_RANGE_CHANGED);
			return;
		}

		LOG_HEXDUMP_DBG(db_hash.hash, sizeof(db_hash.hash), "New Hash: ");

		/* GATT database has been modified since last boot, likely due
		 * to a firmware update or a dynamic service that was not
		 * re-registered on boot.
		 * Indicate Service Changed to all bonded devices for the full
		 * database range to invalidate client-side cache and force
		 * discovery on reconnect.
		 */
		sc_indicate(0x0001, 0xffff);

		/* Hash did not match, overwrite with current hash.
		 * Also immediately set all peers (in settings) as
		 * change-unaware.
		 */
		set_all_change_unaware();
		db_hash_store();
	}
#endif /* defined(CONFIG_BT_SETTINGS) */
}

static void db_hash_process(struct k_work *work)
{
	do_db_hash();
}

static ssize_t db_hash_read(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr,
			    void *buf, uint16_t len, uint16_t offset)
{
	struct gatt_cf_cfg *cfg;

	/* Check if db_hash is already pending in which case it shall be
	 * generated immediately instead of waiting for the work to complete.
	 */
	(void)k_work_cancel_delayable_sync(&db_hash.work, &db_hash.sync);
	if (!atomic_test_bit(gatt_sc.flags, DB_HASH_VALID)) {
		db_hash_gen();
		if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
			set_all_change_unaware();
			db_hash_store();
		}
	}

	/* BLUETOOTH CORE SPECIFICATION Version 5.1 | Vol 3, Part G page 2347:
	 * 2.5.2.1 Robust Caching
	 * A connected client becomes change-aware when...
	 * The client reads the Database Hash characteristic and then the server
	 * receives another ATT request from the client.
	 */
	cfg = find_cf_cfg(conn);
	if (cfg &&
	    CF_ROBUST_CACHING(cfg) &&
	    !atomic_test_bit(cfg->flags, CF_CHANGE_AWARE)) {
		atomic_set_bit(cfg->flags, CF_DB_HASH_READ);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, db_hash.hash,
				 sizeof(db_hash.hash));
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
	if (!bt_le_bond_exists(conn->id, &conn->le.dst)) {
		clear_cf_cfg(cfg);
	} else {
		/* Update address in case it has changed */
		bt_addr_le_copy(&cfg->peer, &conn->le.dst);
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

static struct gatt_cf_cfg *find_cf_cfg_by_addr(uint8_t id,
					       const bt_addr_le_t *addr);

static int bt_gatt_store_cf(uint8_t id, const bt_addr_le_t *peer)
{
#if defined(CONFIG_BT_GATT_CACHING)
	struct gatt_cf_cfg *cfg;
	char dst[CF_NUM_BYTES + CF_FLAGS_STORE_LEN];
	char *str;
	size_t len;
	int err;

	cfg = find_cf_cfg_by_addr(id, peer);
	if (!cfg) {
		/* No cfg found, just clear it */
		LOG_DBG("No config for CF");
		str = NULL;
		len = 0;
	} else {
		str = (char *)cfg->data;
		len = sizeof(cfg->data);

		/* add the CF data to a temp array */
		memcpy(dst, str, len);

		/* add the change-aware flag */
		bool is_change_aware = atomic_test_bit(cfg->flags, CF_CHANGE_AWARE);

		dst[len] = 0;
		WRITE_BIT(dst[len], CF_CHANGE_AWARE, is_change_aware);
		len += CF_FLAGS_STORE_LEN;

		str = dst;
	}

	err = bt_settings_store_cf(id, peer, str, len);
	if (err) {
		LOG_ERR("Failed to store Client Features (err %d)", err);
		return err;
	}

	LOG_DBG("Stored CF for %s", bt_addr_le_str(peer));
	LOG_HEXDUMP_DBG(str, len, "Saved data");
#endif /* CONFIG_BT_GATT_CACHING */
	return 0;

}

static bool is_host_managed_ccc(const struct bt_gatt_attr *attr)
{
	return (attr->write == bt_gatt_attr_write_ccc);
}

#if defined(CONFIG_BT_SETTINGS)
static int gatt_store_ccc(uint8_t id, const bt_addr_le_t *addr);
#else
static int gatt_store_ccc(uint8_t id, const bt_addr_le_t *addr)
{
	/* This function shall never be used without CONFIG_BT_SETTINGS. */
	__ASSERT_NO_MSG(false);

	return -ENOTSUP;
}
#endif /* CONFIG_BT_SETTINGS */


#if defined(CONFIG_BT_SETTINGS) && defined(CONFIG_BT_SMP)
/** Struct used to store both the id and the random address of a device when replacing
 * random addresses in the ccc attribute's cfg array with the device's id address after
 * pairing complete.
 */
struct addr_match {
	const bt_addr_le_t *private_addr;
	const bt_addr_le_t *id_addr;
};

static uint8_t convert_to_id_on_match(const struct bt_gatt_attr *attr,
				      uint16_t handle, void *user_data)
{
	struct bt_gatt_ccc_managed_user_data *ccc;
	struct addr_match *match = user_data;

	if (!is_host_managed_ccc(attr)) {
		return BT_GATT_ITER_CONTINUE;
	}

	ccc = attr->user_data;

	/* Copy the device's id address to the config's address if the config's address is the
	 * same as the device's private address
	 */
	for (size_t i = 0; i < ARRAY_SIZE(ccc->cfg); i++) {
		if (bt_addr_le_eq(&ccc->cfg[i].peer, match->private_addr)) {
			bt_addr_le_copy(&ccc->cfg[i].peer, match->id_addr);
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

static void bt_gatt_identity_resolved(struct bt_conn *conn, const bt_addr_le_t *private_addr,
				      const bt_addr_le_t *id_addr)
{
	/* Update the ccc cfg addresses */
	struct addr_match user_data = {
		.private_addr = private_addr,
		.id_addr      = id_addr
	};
	bool is_bonded = bt_le_bond_exists(conn->id, &conn->le.dst);

	bt_gatt_foreach_attr(0x0001, 0xffff, convert_to_id_on_match, &user_data);

	/* Store the ccc */
	if (is_bonded) {
		gatt_store_ccc(conn->id, &conn->le.dst);
	}

	/* Update the cf addresses and store it if we get a match */
	struct gatt_cf_cfg *cfg = find_cf_cfg_by_addr(conn->id, private_addr);

	if (cfg) {
		bt_addr_le_copy(&cfg->peer, id_addr);
		if (is_bonded) {
			bt_gatt_store_cf(conn->id, &conn->le.dst);
		}
	}
}

static void bt_gatt_pairing_complete(struct bt_conn *conn, bool bonded)
{
	if (bonded) {
		/* Store the ccc and cf data */
		gatt_store_ccc(conn->id, &(conn->le.dst));
		bt_gatt_store_cf(conn->id, &conn->le.dst);
	}
}
#endif /* CONFIG_BT_SETTINGS && CONFIG_BT_SMP */

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
		attrs->_auto_assigned_handle = 0;
		if (!attrs->handle) {
			/* Allocate handle if not set already */
			attrs->handle = ++handle;
			attrs->_auto_assigned_handle = 1;
		} else if (attrs->handle > handle) {
			/* Use existing handle if valid */
			handle = attrs->handle;
		} else if (find_attr(attrs->handle)) {
			/* Service has conflicting handles */
			LOG_ERR("Unable to register handle 0x%04x", attrs->handle);
			return -EINVAL;
		}

		LOG_DBG("attr %p handle 0x%04x uuid %s perm 0x%02x", attrs, attrs->handle,
			bt_uuid_str(attrs->uuid), attrs->perm);
	}

	gatt_insert(svc, last_handle);

	return 0;
}
#endif /* CONFIG_BT_GATT_DYNAMIC_DB */

static inline void sc_work_submit(k_timeout_t timeout)
{
#if defined(CONFIG_BT_GATT_SERVICE_CHANGED)
	k_work_reschedule(&gatt_sc.work, timeout);
#endif
}

#if defined(CONFIG_BT_GATT_SERVICE_CHANGED)
static void sc_indicate_rsp(struct bt_conn *conn,
			    struct bt_gatt_indicate_params *params, uint8_t err)
{
#if defined(CONFIG_BT_GATT_CACHING)
	struct gatt_cf_cfg *cfg;
#endif

	LOG_DBG("err 0x%02x", err);

	atomic_clear_bit(gatt_sc.flags, SC_INDICATE_PENDING);

	/* Check if there is new change in the meantime */
	if (atomic_test_bit(gatt_sc.flags, SC_RANGE_CHANGED)) {
		/* Reschedule without any delay since it is waiting already */
		sc_work_submit(K_NO_WAIT);
	}

#if defined(CONFIG_BT_GATT_CACHING)
	/* BLUETOOTH CORE SPECIFICATION Version 5.3 | Vol 3, Part G page 1476:
	 * 2.5.2.1 Robust Caching
	 * ... a change-unaware connected client using exactly one ATT bearer
	 * becomes change-aware when ...
	 * The client receives and confirms a Handle Value Indication
	 * for the Service Changed characteristic
	 */
	if (bt_att_fixed_chan_only(conn)) {
		cfg = find_cf_cfg(conn);
		if (cfg && CF_ROBUST_CACHING(cfg)) {
			set_change_aware(cfg, true);
		}
	}
#endif /* CONFIG_BT_GATT_CACHING */
}

static void sc_process(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct gatt_sc *sc = CONTAINER_OF(dwork, struct gatt_sc, work);
	uint16_t sc_range[2];

	__ASSERT(!atomic_test_bit(sc->flags, SC_INDICATE_PENDING),
		 "Indicate already pending");

	LOG_DBG("start 0x%04x end 0x%04x", sc->start, sc->end);

	sc_range[0] = sys_cpu_to_le16(sc->start);
	sc_range[1] = sys_cpu_to_le16(sc->end);

	atomic_clear_bit(sc->flags, SC_RANGE_CHANGED);
	sc->start = 0U;
	sc->end = 0U;

	sc->params.attr = &_1_gatt_svc.attrs[2];
	sc->params.func = sc_indicate_rsp;
	sc->params.data = &sc_range[0];
	sc->params.len = sizeof(sc_range);
#if defined(CONFIG_BT_EATT)
	sc->params.chan_opt = BT_ATT_CHAN_OPT_NONE;
#endif /* CONFIG_BT_EATT */

	if (bt_gatt_indicate(NULL, &sc->params)) {
		/* No connections to indicate */
		return;
	}

	atomic_set_bit(sc->flags, SC_INDICATE_PENDING);
}
#endif /* defined(CONFIG_BT_GATT_SERVICE_CHANGED) */

static void clear_ccc_cfg(struct bt_gatt_ccc_cfg *cfg)
{
	bt_addr_le_copy(&cfg->peer, BT_ADDR_LE_ANY);
	cfg->id = 0U;
	cfg->value = 0U;
}

static void gatt_store_ccc_cf(uint8_t id, const bt_addr_le_t *peer_addr);

struct ds_peer {
	uint8_t id;
	bt_addr_le_t peer;

	ATOMIC_DEFINE(flags, DELAYED_STORE_NUM_FLAGS);
};

IF_ENABLED(CONFIG_BT_SETTINGS_DELAYED_STORE, (
static struct gatt_delayed_store {
	struct ds_peer peer_list[CONFIG_BT_MAX_PAIRED + CONFIG_BT_MAX_CONN];
	struct k_work_delayable work;
} gatt_delayed_store;
))

static struct ds_peer *gatt_delayed_store_find(uint8_t id,
					       const bt_addr_le_t *peer_addr)
{
	IF_ENABLED(CONFIG_BT_SETTINGS_DELAYED_STORE, ({
		struct ds_peer *el;

		for (size_t i = 0; i < ARRAY_SIZE(gatt_delayed_store.peer_list); i++) {
			el = &gatt_delayed_store.peer_list[i];
			if (el->id == id &&
			bt_addr_le_eq(peer_addr, &el->peer)) {
				return el;
			}
		}
	}))

	return NULL;
}

static void gatt_delayed_store_free(struct ds_peer *el)
{
	if (el) {
		el->id = 0;
		memset(&el->peer, 0, sizeof(el->peer));
		atomic_set(el->flags, 0);
	}
}

#if defined(CONFIG_BT_SETTINGS_DELAYED_STORE)
static struct ds_peer *gatt_delayed_store_alloc(uint8_t id,
						const bt_addr_le_t *peer_addr)
{
	struct ds_peer *el;

	for (size_t i = 0; i < ARRAY_SIZE(gatt_delayed_store.peer_list); i++) {
		el = &gatt_delayed_store.peer_list[i];

		/* Checking for the flags is cheaper than a memcmp for the
		 * address, so we use that to signal that a given slot is
		 * free.
		 */
		if (atomic_get(el->flags) == 0) {
			bt_addr_le_copy(&el->peer, peer_addr);
			el->id = id;

			return el;
		}
	}

	return NULL;
}

static void gatt_delayed_store_enqueue(uint8_t id, const bt_addr_le_t *peer_addr,
				       enum delayed_store_flags flag)
{
	bool bonded = bt_le_bond_exists(id, peer_addr);
	struct ds_peer *el = gatt_delayed_store_find(id, peer_addr);

	if (bonded) {
		if (el == NULL) {
			el = gatt_delayed_store_alloc(id, peer_addr);
			__ASSERT(el != NULL, "Can't save CF / CCC to flash");
		}

		atomic_set_bit(el->flags, flag);

		k_work_reschedule(&gatt_delayed_store.work,
				  K_MSEC(CONFIG_BT_SETTINGS_DELAYED_STORE_MS));
	}
}

static void delayed_store(struct k_work *work)
{
	struct ds_peer *el;
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct gatt_delayed_store *store =
		CONTAINER_OF(dwork, struct gatt_delayed_store, work);

	for (size_t i = 0; i < ARRAY_SIZE(gatt_delayed_store.peer_list); i++) {
		el = &store->peer_list[i];

		gatt_store_ccc_cf(el->id, &el->peer);
	}
}
#endif	/* CONFIG_BT_SETTINGS_DELAYED_STORE */

static void gatt_store_ccc_cf(uint8_t id, const bt_addr_le_t *peer_addr)
{
	struct ds_peer *el = gatt_delayed_store_find(id, peer_addr);

	if (bt_le_bond_exists(id, peer_addr)) {
		if (!IS_ENABLED(CONFIG_BT_SETTINGS_CCC_STORE_ON_WRITE) ||
		    (IS_ENABLED(CONFIG_BT_SETTINGS_CCC_STORE_ON_WRITE) && el &&
		     atomic_test_and_clear_bit(el->flags, DELAYED_STORE_CCC))) {
			gatt_store_ccc(id, peer_addr);
		}

		if (!IS_ENABLED(CONFIG_BT_SETTINGS_CF_STORE_ON_WRITE) ||
		    (IS_ENABLED(CONFIG_BT_SETTINGS_CF_STORE_ON_WRITE) && el &&
		     atomic_test_and_clear_bit(el->flags, DELAYED_STORE_CF))) {
			bt_gatt_store_cf(id, peer_addr);
		}

		if (el && atomic_get(el->flags) == 0) {
			gatt_delayed_store_free(el);
		}
	}
}

#if defined(CONFIG_BT_SETTINGS) && defined(CONFIG_BT_SMP)
BT_CONN_CB_DEFINE(gatt_conn_cb) = {
	/* Also update the address of CCC or CF writes that happened before the
	 * identity resolution. Note that to increase security in the future, we
	 * might want to explicitly not do this and treat a bonded device as a
	 * brand-new peer.
	 */
	.identity_resolved = bt_gatt_identity_resolved,
};
#endif /* CONFIG_BT_SETTINGS && CONFIG_BT_SMP */

static void bt_gatt_service_init(void)
{
	if (atomic_test_and_set_bit(gatt_flags, GATT_SERVICE_INITIALIZED)) {
		return;
	}

	STRUCT_SECTION_FOREACH(bt_gatt_service_static, svc) {
		last_static_handle += svc->attr_count;
	}
}

void bt_gatt_init(void)
{
	if (atomic_test_and_set_bit(gatt_flags, GATT_INITIALIZED)) {
		return;
	}

	bt_gatt_service_init();

#if defined(CONFIG_BT_GATT_CACHING)
	k_work_init_delayable(&db_hash.work, db_hash_process);

	/* Submit work to Generate initial hash as there could be static
	 * services already in the database.
	 */
	if (IS_ENABLED(CONFIG_BT_LONG_WQ)) {
		bt_long_wq_schedule(&db_hash.work, DB_HASH_TIMEOUT);
	} else {
		k_work_schedule(&db_hash.work, DB_HASH_TIMEOUT);
	}
#endif /* CONFIG_BT_GATT_CACHING */

#if defined(CONFIG_BT_GATT_SERVICE_CHANGED)
	k_work_init_delayable(&gatt_sc.work, sc_process);
	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		/* Make sure to not send SC indications until SC
		 * settings are loaded
		 */
		atomic_set_bit(gatt_sc.flags, SC_INDICATE_PENDING);
	}
#endif /* defined(CONFIG_BT_GATT_SERVICE_CHANGED) */

#if defined(CONFIG_BT_SETTINGS_DELAYED_STORE)
	k_work_init_delayable(&gatt_delayed_store.work, delayed_store);
#endif

#if defined(CONFIG_BT_SETTINGS) && defined(CONFIG_BT_SMP)
	static struct bt_conn_auth_info_cb gatt_conn_auth_info_cb = {
		.pairing_complete = bt_gatt_pairing_complete,
	};

	/* Register the gatt module for authentication info callbacks so it can
	 * be notified when pairing has completed. This is used to enable CCC
	 * and CF storage on pairing complete.
	 */
	bt_conn_auth_info_cb_register(&gatt_conn_auth_info_cb);
#endif /* CONFIG_BT_SETTINGS && CONFIG_BT_SMP */
}

static void sc_indicate(uint16_t start, uint16_t end)
{
#if defined(CONFIG_BT_GATT_DYNAMIC_DB) ||                                                          \
	(defined(CONFIG_BT_GATT_CACHING) && defined(CONFIG_BT_SETTINGS))
	LOG_DBG("start 0x%04x end 0x%04x", start, end);

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
		LOG_DBG("indicate pending, waiting until complete...");
		return;
	}

	/* Reschedule since the range has changed */
	sc_work_submit(SC_TIMEOUT);
#endif /* BT_GATT_DYNAMIC_DB || (BT_GATT_CACHING && BT_SETTINGS) */
}

void bt_gatt_cb_register(struct bt_gatt_cb *cb)
{
	sys_slist_append(&callback_list, &cb->node);
}

#if defined(CONFIG_BT_GATT_DYNAMIC_DB)
static void db_changed(void)
{
#if defined(CONFIG_BT_GATT_CACHING)
	struct bt_conn *conn;
	int i;

	atomic_clear_bit(gatt_sc.flags, DB_HASH_VALID);

	if (IS_ENABLED(CONFIG_BT_LONG_WQ)) {
		bt_long_wq_reschedule(&db_hash.work, DB_HASH_TIMEOUT);
	} else {
		k_work_reschedule(&db_hash.work, DB_HASH_TIMEOUT);
	}

	for (i = 0; i < ARRAY_SIZE(cf_cfg); i++) {
		struct gatt_cf_cfg *cfg = &cf_cfg[i];

		if (bt_addr_le_eq(&cfg->peer, BT_ADDR_LE_ANY)) {
			continue;
		}

		if (CF_ROBUST_CACHING(cfg)) {
			/* Core Spec 5.1 | Vol 3, Part G, 2.5.2.1 Robust Caching
			 *... the database changes again before the client
			 * becomes change-aware in which case the error response
			 * shall be sent again.
			 */
			conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cfg->peer);
			if (conn) {
				bt_att_clear_out_of_sync_sent(conn);
				bt_conn_unref(conn);
			}

			atomic_clear_bit(cfg->flags, CF_DB_HASH_READ);
			set_change_aware(cfg, false);
		}
	}
#endif
}

static void gatt_unregister_ccc(struct bt_gatt_ccc_managed_user_data *ccc)
{
	ccc->value = 0;

	for (size_t i = 0; i < ARRAY_SIZE(ccc->cfg); i++) {
		struct bt_gatt_ccc_cfg *cfg = &ccc->cfg[i];

		if (!bt_addr_le_eq(&cfg->peer, BT_ADDR_LE_ANY)) {
			struct bt_conn *conn;
			bool store = true;

			conn = bt_conn_lookup_addr_le(cfg->id, &cfg->peer);
			if (conn) {
				if (conn->state == BT_CONN_CONNECTED) {
#if defined(CONFIG_BT_SETTINGS_CCC_STORE_ON_WRITE)
					gatt_delayed_store_enqueue(conn->id,
								   &conn->le.dst,
								   DELAYED_STORE_CCC);
#endif
					store = false;
				}

				bt_conn_unref(conn);
			}

			if (IS_ENABLED(CONFIG_BT_SETTINGS) && store &&
			    bt_le_bond_exists(cfg->id, &cfg->peer)) {
				gatt_store_ccc(cfg->id, &cfg->peer);
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

		if (is_host_managed_ccc(attr)) {
			gatt_unregister_ccc(attr->user_data);
		}

		/* The stack should not clear any handles set by the user. */
		if (attr->_auto_assigned_handle) {
			attr->handle = 0;
			attr->_auto_assigned_handle = 0;
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

	if (IS_ENABLED(CONFIG_BT_SETTINGS) &&
	    atomic_test_bit(gatt_flags, GATT_INITIALIZED) &&
	    !atomic_test_bit(gatt_sc.flags, SC_LOAD)) {
		LOG_DBG("Can't register service after init and before settings are loaded.");
		return -EINVAL;
	}

	/* Init GATT core services */
	bt_gatt_service_init();

	/* Do no allow to register mandatory services twice */
	if (!bt_uuid_cmp(svc->attrs[0].uuid, BT_UUID_GAP) ||
	    !bt_uuid_cmp(svc->attrs[0].uuid, BT_UUID_GATT)) {
		return -EALREADY;
	}

	k_sched_lock();

	err = gatt_register(svc);
	if (err < 0) {
		k_sched_unlock();
		return err;
	}

	/* Don't submit any work until the stack is initialized */
	if (!atomic_test_bit(gatt_flags, GATT_INITIALIZED)) {
		k_sched_unlock();
		return 0;
	}

	sc_indicate(svc->attrs[0].handle,
		    svc->attrs[svc->attr_count - 1].handle);

	db_changed();

	k_sched_unlock();

	return 0;
}

int bt_gatt_service_unregister(struct bt_gatt_service *svc)
{
	uint16_t sc_start_handle;
	uint16_t sc_end_handle;
	int err;

	__ASSERT(svc, "invalid parameters\n");

	/* gatt_unregister() clears handles when those were auto-assigned
	 * by host
	 */
	sc_start_handle = svc->attrs[0].handle;
	sc_end_handle = svc->attrs[svc->attr_count - 1].handle;

	k_sched_lock();

	err = gatt_unregister(svc);
	if (err) {
		k_sched_unlock();
		return err;
	}

	/* Don't submit any work until the stack is initialized */
	if (!atomic_test_bit(gatt_flags, GATT_INITIALIZED)) {
		k_sched_unlock();
		return 0;
	}

	sc_indicate(sc_start_handle, sc_end_handle);

	db_changed();

	k_sched_unlock();

	return 0;
}

bool bt_gatt_service_is_registered(const struct bt_gatt_service *svc)
{
	bool registered = false;
	sys_snode_t *node;

	k_sched_lock();
	SYS_SLIST_FOR_EACH_NODE(&db, node) {
		if (&svc->node == node) {
			registered = true;
			break;
		}
	}

	k_sched_unlock();

	return registered;
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

	if (value_len != 0U && value == NULL) {
		LOG_DBG("value_len of %u provided for NULL value", value_len);

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	if (value_len == 0U) {
		len = 0U;
	} else {
		len = MIN(buf_len, value_len - offset);
		memcpy(buf, (uint8_t *)value + offset, len);
	}

	LOG_DBG("handle 0x%04x offset %u length %u", attr->handle, offset, len);

	return len;
}

ssize_t bt_gatt_attr_read_service(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  void *buf, uint16_t len, uint16_t offset)
{
	const struct bt_uuid *uuid = attr->user_data;

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

	include->end_handle = sys_cpu_to_le16(handle);

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

	STRUCT_SECTION_FOREACH(bt_gatt_service_static, static_svc) {
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
	if ((attr == NULL) || (attr->user_data == NULL)) {
		return -EINVAL;
	}

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
	} __packed;
} __packed;

uint16_t bt_gatt_attr_value_handle(const struct bt_gatt_attr *attr)
{
	uint16_t handle = 0;

	if (attr != NULL && bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CHRC) == 0) {
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

		STRUCT_SECTION_FOREACH(bt_gatt_service_static, static_svc) {
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
					    struct bt_gatt_ccc_managed_user_data *ccc)
{
	for (size_t i = 0; i < ARRAY_SIZE(ccc->cfg); i++) {
		struct bt_gatt_ccc_cfg *cfg = &ccc->cfg[i];

		if (conn) {
			if (bt_conn_is_peer_addr_le(conn, cfg->id,
						    &cfg->peer)) {
				return cfg;
			}
		} else if (bt_addr_le_eq(&cfg->peer, BT_ADDR_LE_ANY)) {
			return cfg;
		}
	}

	return NULL;
}

ssize_t bt_gatt_attr_read_ccc(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, void *buf,
			      uint16_t len, uint16_t offset)
{
	struct bt_gatt_ccc_managed_user_data *ccc = attr->user_data;
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
			     struct bt_gatt_ccc_managed_user_data *ccc)
{
	int i;
	uint16_t value = 0x0000;

	for (i = 0; i < ARRAY_SIZE(ccc->cfg); i++) {
		/* `ccc->value` shall be a summary of connected peers' CCC values, but
		 * `ccc->cfg` can contain entries for bonded but not connected peers.
		 */
		struct bt_conn *conn = bt_conn_lookup_addr_le(ccc->cfg[i].id, &ccc->cfg[i].peer);

		if (conn) {
			if (ccc->cfg[i].value > value) {
				value = ccc->cfg[i].value;
			}

			bt_conn_unref(conn);
		}
	}

	LOG_DBG("ccc %p value 0x%04x", ccc, value);

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
	struct bt_gatt_ccc_managed_user_data *ccc = attr->user_data;
	struct bt_gatt_ccc_cfg *cfg;
	bool value_changed;
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
		 * behaviorally the same as no written CCC.
		 */
		if (!value) {
			return len;
		}

		cfg = find_ccc_cfg(NULL, ccc);
		if (!cfg) {
			LOG_WRN("No space to store CCC cfg");
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

	value_changed = cfg->value != value;
	cfg->value = value;

	LOG_DBG("handle 0x%04x value %u", attr->handle, cfg->value);

	/* Update cfg if don't match */
	if (cfg->value != ccc->value) {
		gatt_ccc_changed(attr, ccc);
	}

	if (value_changed) {
#if defined(CONFIG_BT_SETTINGS_CCC_STORE_ON_WRITE)
		/* Enqueue CCC store if value has changed for the connection */
		gatt_delayed_store_enqueue(conn->id, &conn->le.dst, DELAYED_STORE_CCC);
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

struct gatt_cpf {
	uint8_t  format;
	int8_t   exponent;
	uint16_t unit;
	uint8_t  name_space;
	uint16_t description;
} __packed;

ssize_t bt_gatt_attr_read_cpf(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, void *buf,
			      uint16_t len, uint16_t offset)
{
	const struct bt_gatt_cpf *cpf = attr->user_data;
	struct gatt_cpf value;

	value.format = cpf->format;
	value.exponent = cpf->exponent;
	value.unit = sys_cpu_to_le16(cpf->unit);
	value.name_space = cpf->name_space;
	value.description = sys_cpu_to_le16(cpf->description);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &value,
				 sizeof(value));
}

struct notify_data {
	const struct bt_gatt_attr *attr;
	uint16_t handle;
	int err;
	uint16_t type;
	union {
		struct bt_gatt_notify_params *nfy_params;
		struct bt_gatt_indicate_params *ind_params;
	};
};

#if defined(CONFIG_BT_GATT_NOTIFY_MULTIPLE)

static struct net_buf *nfy_mult[CONFIG_BT_MAX_CONN];

static int gatt_notify_mult_send(struct bt_conn *conn, struct net_buf *buf)
{
	int ret;
	uint8_t *pdu = buf->data;
	/* PDU structure is [Opcode (1)] [Handle (2)] [Length (2)] [Value (Length)] */
	uint16_t first_attr_len = sys_get_le16(&pdu[3]);

	/* Convert to ATT_HANDLE_VALUE_NTF if containing a single handle. */
	if (buf->len ==
	    (1 + sizeof(struct bt_att_notify_mult) + first_attr_len)) {
		/* Store attr handle */
		uint16_t handle = sys_get_le16(&pdu[1]);

		/* Remove the ATT_MULTIPLE_HANDLE_VALUE_NTF opcode,
		 * attribute handle and length
		 */
		(void)net_buf_pull(buf, 1 + sizeof(struct bt_att_notify_mult));

		/* Add back an ATT_HANDLE_VALUE_NTF opcode and attr handle */
		/* PDU structure is now [Opcode (1)] [Handle (1)] [Value] */
		net_buf_push_le16(buf, handle);
		net_buf_push_u8(buf, BT_ATT_OP_NOTIFY);
		LOG_DBG("Converted BT_ATT_OP_NOTIFY_MULT with single attr to BT_ATT_OP_NOTIFY");
	}

	ret = bt_att_send(conn, buf);
	if (ret < 0) {
		net_buf_unref(buf);
	}

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

			gatt_notify_mult_send(conn, *buf);
			*buf = NULL;
			bt_conn_unref(conn);
		}
	}
}

K_WORK_DELAYABLE_DEFINE(nfy_mult_work, notify_mult_process);

static bool gatt_cf_notify_multi(struct bt_conn *conn)
{
	struct gatt_cf_cfg *cfg;

	cfg = find_cf_cfg(conn);
	if (!cfg) {
		return false;
	}

	return CF_NOTIFY_MULTI(cfg);
}

static int gatt_notify_flush(struct bt_conn *conn)
{
	int err = 0;
	struct net_buf **buf = &nfy_mult[bt_conn_index(conn)];

	if (*buf) {
		err = gatt_notify_mult_send(conn, *buf);
		*buf = NULL;
	}

	return err;
}

static void cleanup_notify(struct bt_conn *conn)
{
	struct net_buf **buf = &nfy_mult[bt_conn_index(conn)];

	if (*buf) {
		net_buf_unref(*buf);
		*buf = NULL;
	}
}

static void gatt_add_nfy_to_buf(struct net_buf *buf,
				uint16_t handle,
				struct bt_gatt_notify_params *params)
{
	struct bt_att_notify_mult *nfy;

	nfy = net_buf_add(buf, sizeof(*nfy) + params->len);
	nfy->handle = sys_cpu_to_le16(handle);
	nfy->len = sys_cpu_to_le16(params->len);
	(void)memcpy(nfy->value, params->data, params->len);
}

#if (CONFIG_BT_GATT_NOTIFY_MULTIPLE_FLUSH_MS != 0)
static int gatt_notify_mult(struct bt_conn *conn, uint16_t handle,
			    struct bt_gatt_notify_params *params)
{
	struct net_buf **buf = &nfy_mult[bt_conn_index(conn)];

	/* Check if we can fit more data into it, in case it doesn't fit send
	 * the existing buffer and proceed to create a new one
	 */
	if (*buf && ((net_buf_tailroom(*buf) < sizeof(struct bt_att_notify_mult) + params->len) ||
	    !bt_att_tx_meta_data_match(*buf, params->func, params->user_data,
				       BT_ATT_CHAN_OPT(params)))) {
		int ret;

		ret = gatt_notify_mult_send(conn, *buf);
		*buf = NULL;
		if (ret < 0) {
			return ret;
		}
	}

	if (!*buf) {
		*buf = bt_att_create_pdu(conn, BT_ATT_OP_NOTIFY_MULT,
					 sizeof(struct bt_att_notify_mult) + params->len);
		if (!*buf) {
			return -ENOMEM;
		}

		bt_att_set_tx_meta_data(*buf, params->func, params->user_data,
					BT_ATT_CHAN_OPT(params));
	} else {
		/* Increment the number of handles, ensuring the notify callback
		 * gets called once for every attribute.
		 */
		bt_att_increment_tx_meta_data_attr_count(*buf, 1);
	}

	LOG_DBG("handle 0x%04x len %u", handle, params->len);
	gatt_add_nfy_to_buf(*buf, handle, params);

	/* Use `k_work_schedule` to keep the original deadline, instead of
	 * re-setting the timeout whenever a new notification is appended.
	 */
	k_work_schedule(&nfy_mult_work,
			K_MSEC(CONFIG_BT_GATT_NOTIFY_MULTIPLE_FLUSH_MS));

	return 0;
}
#endif /* CONFIG_BT_GATT_NOTIFY_MULTIPLE_FLUSH_MS != 0 */
#endif /* CONFIG_BT_GATT_NOTIFY_MULTIPLE */

static int gatt_notify(struct bt_conn *conn, uint16_t handle,
		       struct bt_gatt_notify_params *params)
{
	struct net_buf *buf;
	struct bt_att_notify *nfy;

#if defined(CONFIG_BT_GATT_ENFORCE_CHANGE_UNAWARE)
	/* BLUETOOTH CORE SPECIFICATION Version 5.3
	 * Vol 3, Part G 2.5.3 (page 1479):
	 *
	 * Except for a Handle Value indication for the Service Changed
	 * characteristic, the server shall not send notifications and
	 * indications to such a client until it becomes change-aware.
	 */
	if (!bt_gatt_change_aware(conn, false)) {
		return -EAGAIN;
	}
#endif

	/* Confirm that the connection has the correct level of security */
	if (bt_gatt_check_perm(conn, params->attr, BT_GATT_PERM_READ_ENCRYPT_MASK)) {
		LOG_DBG("Link is not encrypted");
		return -EPERM;
	}

	if (IS_ENABLED(CONFIG_BT_GATT_ENFORCE_SUBSCRIPTION)) {
		/* Check if client has subscribed before sending notifications.
		 * This is not really required in the Bluetooth specification,
		 * but follows its spirit.
		 */
		if (!bt_gatt_is_subscribed(conn, params->attr, BT_GATT_CCC_NOTIFY)) {
			LOG_DBG("Device is not subscribed to characteristic");
			return -EINVAL;
		}
	}

	if (IS_ENABLED(CONFIG_BT_EATT) &&
	    !bt_att_chan_opt_valid(conn, BT_ATT_CHAN_OPT(params))) {
		return -EINVAL;
	}

#if defined(CONFIG_BT_GATT_NOTIFY_MULTIPLE) && (CONFIG_BT_GATT_NOTIFY_MULTIPLE_FLUSH_MS != 0)
	if (gatt_cf_notify_multi(conn)) {
		return gatt_notify_mult(conn, handle, params);
	}
#endif /* CONFIG_BT_GATT_NOTIFY_MULTIPLE */

	buf = bt_att_create_pdu(conn, BT_ATT_OP_NOTIFY,
				sizeof(*nfy) + params->len);
	if (!buf) {
		LOG_DBG("No buffer available to send notification");
		return -ENOMEM;
	}

	LOG_DBG("conn %p handle 0x%04x", conn, handle);

	nfy = net_buf_add(buf, sizeof(*nfy) + params->len);
	nfy->handle = sys_cpu_to_le16(handle);
	memcpy(nfy->value, params->data, params->len);

	bt_att_set_tx_meta_data(buf, params->func, params->user_data, BT_ATT_CHAN_OPT(params));
	return bt_att_send(conn, buf);
}

/* Converts error (negative errno) to ATT Error code */
static uint8_t att_err_from_int(int err)
{
	LOG_DBG("%d", err);

	/* ATT error codes are 1 byte values, so any value outside the range is unknown */
	if (!IN_RANGE(err, 0, UINT8_MAX)) {
		return BT_ATT_ERR_UNLIKELY;
	}

	return err;
}

static void gatt_indicate_rsp(struct bt_conn *conn, int err,
			      const void *pdu, uint16_t length, void *user_data)
{
	struct bt_gatt_indicate_params *params = user_data;

	if (params->func) {
		params->func(conn, params, att_err_from_int(err));
	}

	params->_ref--;
	if (params->destroy && (params->_ref == 0)) {
		params->destroy(params);
	}
}

static struct bt_att_req *gatt_req_alloc(bt_att_func_t func, void *params,
					 bt_att_encode_t encode,
					 uint8_t op,
					 size_t len)
{
	struct bt_att_req *req;

	/* Allocate new request */
	req = bt_att_req_alloc(BT_ATT_TIMEOUT);
	if (!req) {
		return NULL;
	}

#if defined(CONFIG_BT_SMP)
	req->att_op = op;
	req->len = len;
	req->encode = encode;
#endif
	req->func = func;
	req->user_data = params;

	return req;
}

#ifdef CONFIG_BT_GATT_CLIENT
static int gatt_req_send(struct bt_conn *conn, bt_att_func_t func, void *params,
			 bt_att_encode_t encode, uint8_t op, size_t len,
			 enum bt_att_chan_opt chan_opt)

{
	struct bt_att_req *req;
	struct net_buf *buf;
	int err;

	if (IS_ENABLED(CONFIG_BT_EATT) &&
	    !bt_att_chan_opt_valid(conn, chan_opt)) {
		return -EINVAL;
	}

	req = gatt_req_alloc(func, params, encode, op, len);
	if (!req) {
		return -ENOMEM;
	}

	buf = bt_att_create_pdu(conn, op, len);
	if (!buf) {
		bt_att_req_free(req);
		return -ENOMEM;
	}

	bt_att_set_tx_meta_data(buf, NULL, NULL, chan_opt);

	req->buf = buf;

	err = encode(buf, len, params);
	if (err) {
		bt_att_req_free(req);
		return err;
	}

	err = bt_att_req_send(conn, req);
	if (err) {
		bt_att_req_free(req);
	}

	return err;
}
#endif

static int gatt_indicate(struct bt_conn *conn, uint16_t handle,
			 struct bt_gatt_indicate_params *params)
{
	struct net_buf *buf;
	struct bt_att_indicate *ind;
	struct bt_att_req *req;
	size_t len;
	int err;

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

	/* Confirm that the connection has the correct level of security */
	if (bt_gatt_check_perm(conn, params->attr, BT_GATT_PERM_READ_ENCRYPT_MASK)) {
		LOG_DBG("Link is not encrypted");
		return -EPERM;
	}

	if (IS_ENABLED(CONFIG_BT_GATT_ENFORCE_SUBSCRIPTION)) {
		/* Check if client has subscribed before sending notifications.
		 * This is not really required in the Bluetooth specification,
		 * but follows its spirit.
		 */
		if (!bt_gatt_is_subscribed(conn, params->attr, BT_GATT_CCC_INDICATE)) {
			LOG_DBG("Device is not subscribed to characteristic");
			return -EINVAL;
		}
	}

	if (IS_ENABLED(CONFIG_BT_EATT) &&
	    !bt_att_chan_opt_valid(conn, BT_ATT_CHAN_OPT(params))) {
		return -EINVAL;
	}

	len = sizeof(*ind) + params->len;

	req = gatt_req_alloc(gatt_indicate_rsp, params, NULL,
			     BT_ATT_OP_INDICATE, len);
	if (!req) {
		return -ENOMEM;
	}

	buf = bt_att_create_pdu(conn, BT_ATT_OP_INDICATE, len);
	if (!buf) {
		LOG_DBG("No buffer available to send indication");
		bt_att_req_free(req);
		return -ENOMEM;
	}

	bt_att_set_tx_meta_data(buf, NULL, NULL, BT_ATT_CHAN_OPT(params));

	ind = net_buf_add(buf, sizeof(*ind) + params->len);
	ind->handle = sys_cpu_to_le16(handle);
	memcpy(ind->value, params->data, params->len);

	LOG_DBG("conn %p handle 0x%04x", conn, handle);

	req->buf = buf;

	err = bt_att_req_send(conn, req);
	if (err) {
		bt_att_req_free(req);
	}

	return err;
}

static uint8_t notify_cb(const struct bt_gatt_attr *attr, uint16_t handle,
			 void *user_data)
{
	struct notify_data *data = user_data;
	struct bt_gatt_ccc_managed_user_data *ccc;
	size_t i;

	if (!is_host_managed_ccc(attr)) {
		return BT_GATT_ITER_CONTINUE;
	}

	ccc = attr->user_data;

	/* Save Service Changed data if peer is not connected */
	if (IS_ENABLED(CONFIG_BT_GATT_SERVICE_CHANGED) && ccc == &sc_ccc) {
		for (i = 0; i < ARRAY_SIZE(sc_cfg); i++) {
			struct gatt_sc_cfg *cfg = &sc_cfg[i];
			struct bt_conn *conn;

			if (bt_addr_le_eq(&cfg->peer, BT_ADDR_LE_ANY)) {
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

		/* Confirm that the connection has the correct level of security */
		if (bt_gatt_check_perm(conn, attr, BT_GATT_PERM_READ_ENCRYPT_MASK)) {
			LOG_DBG("Link %p is not encrypted", (void *)conn);
			bt_conn_unref(conn);
			continue;
		}

		/* Use the Characteristic Value handle discovered since the
		 * Client Characteristic Configuration descriptor may occur
		 * in any position within the characteristic definition after
		 * the Characteristic Value.
		 * Only notify or indicate devices which are subscribed.
		 */
		if ((data->type == BT_GATT_CCC_INDICATE) &&
		    (cfg->value & BT_GATT_CCC_INDICATE)) {
			err = gatt_indicate(conn, data->handle, data->ind_params);
			if (err == 0) {
				data->ind_params->_ref++;
			}
		} else if ((data->type == BT_GATT_CCC_NOTIFY) &&
			   (cfg->value & BT_GATT_CCC_NOTIFY)) {
			err = gatt_notify(conn, data->handle, data->nfy_params);
		} else {
			err = 0;
		}

		bt_conn_unref(conn);

		data->err = err;

		if (err < 0) {
			return BT_GATT_ITER_STOP;
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t match_uuid(const struct bt_gatt_attr *attr, uint16_t handle,
			  void *user_data)
{
	struct notify_data *data = user_data;

	data->attr = attr;
	data->handle = handle;

	return BT_GATT_ITER_STOP;
}

static bool gatt_find_by_uuid(struct notify_data *found,
			      const struct bt_uuid *uuid)
{
	found->attr = NULL;

	bt_gatt_foreach_attr_type(found->handle, 0xffff, uuid, NULL, 1,
				  match_uuid, found);

	return found->attr ? true : false;
}

struct bt_gatt_attr *bt_gatt_find_by_uuid(const struct bt_gatt_attr *attr,
					  uint16_t attr_count,
					  const struct bt_uuid *uuid)
{
	struct bt_gatt_attr *found = NULL;
	uint16_t start_handle = bt_gatt_attr_get_handle(attr);
	uint16_t end_handle = start_handle && attr_count
				      ? MIN(start_handle + attr_count, BT_ATT_LAST_ATTRIBUTE_HANDLE)
				      : BT_ATT_LAST_ATTRIBUTE_HANDLE;

	if (attr != NULL && start_handle == 0U) {
		/* If start_handle is 0 then `attr` is not in our database, and should not be used
		 * as a starting point for the search
		 */
		LOG_DBG("Could not find handle of attr %p", attr);
		return NULL;
	}

	bt_gatt_foreach_attr_type(start_handle, end_handle, uuid, NULL, 1, find_next, &found);

	return found;
}

int bt_gatt_notify_cb(struct bt_conn *conn,
		      struct bt_gatt_notify_params *params)
{
	struct notify_data data;

	__ASSERT(params, "invalid parameters\n");
	__ASSERT(params->attr || params->uuid, "invalid parameters\n");

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		return -EAGAIN;
	}

	if (conn && conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	data.attr = params->attr;
	data.handle = bt_gatt_attr_get_handle(data.attr);

	/* Lookup UUID if it was given */
	if (params->uuid) {
		if (!gatt_find_by_uuid(&data, params->uuid)) {
			return -ENOENT;
		}

		params->attr = data.attr;
	} else {
		if (!data.handle) {
			return -ENOENT;
		}
	}

	/* Check if attribute is a characteristic then adjust the handle */
	if (!bt_uuid_cmp(data.attr->uuid, BT_UUID_GATT_CHRC)) {
		struct bt_gatt_chrc *chrc = data.attr->user_data;

		if (!(chrc->properties & BT_GATT_CHRC_NOTIFY)) {
			return -EINVAL;
		}

		data.handle = bt_gatt_attr_value_handle(data.attr);
	}

	if (conn) {
		return gatt_notify(conn, data.handle, params);
	}

	data.err = -ENOTCONN;
	data.type = BT_GATT_CCC_NOTIFY;
	data.nfy_params = params;

	bt_gatt_foreach_attr_type(data.handle, 0xffff, BT_UUID_GATT_CCC, NULL,
				  1, notify_cb, &data);

	return data.err;
}

#if defined(CONFIG_BT_GATT_NOTIFY_MULTIPLE)
static int gatt_notify_multiple_verify_args(struct bt_conn *conn,
					    struct bt_gatt_notify_params params[],
					    uint16_t num_params)
{
	__ASSERT(params, "invalid parameters\n");
	__ASSERT(params->attr, "invalid parameters\n");

	CHECKIF(num_params < 2) {
		/* Use the standard notification API when sending only one
		 * notification.
		 */
		return -EINVAL;
	}

	CHECKIF(conn == NULL) {
		/* Use the standard notification API to send to all connected
		 * peers.
		 */
		return -EINVAL;
	}

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		return -EAGAIN;
	}

	if (conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

#if defined(CONFIG_BT_GATT_ENFORCE_CHANGE_UNAWARE)
	/* BLUETOOTH CORE SPECIFICATION Version 5.3
	 * Vol 3, Part G 2.5.3 (page 1479):
	 *
	 * Except for a Handle Value indication for the Service Changed
	 * characteristic, the server shall not send notifications and
	 * indications to such a client until it becomes change-aware.
	 */
	if (!bt_gatt_change_aware(conn, false)) {
		return -EAGAIN;
	}
#endif

	/* This API guarantees an ATT_MULTIPLE_HANDLE_VALUE_NTF over the air. */
	if (!gatt_cf_notify_multi(conn)) {
		return -EOPNOTSUPP;
	}

	return 0;
}

static int gatt_notify_multiple_verify_params(struct bt_conn *conn,
					     struct bt_gatt_notify_params params[],
					     uint16_t num_params, size_t *total_len)
{
	for (uint16_t i = 0; i < num_params; i++) {
		/* Compute the total data length. */
		*total_len += params[i].len;

		/* Confirm that the connection has the correct level of security. */
		if (bt_gatt_check_perm(conn, params[i].attr,
				       BT_GATT_PERM_READ_ENCRYPT |
				       BT_GATT_PERM_READ_AUTHEN)) {
			LOG_DBG("Link %p is not encrypted", (void *)conn);
			return -EPERM;
		}

		/* The current implementation requires the same callbacks and
		 * user_data.
		 */
		if ((params[0].func != params[i].func) ||
		    (params[0].user_data != params[i].user_data)) {
			return -EINVAL;
		}

		/* This API doesn't support passing UUIDs. */
		if (params[i].uuid) {
			return -EINVAL;
		}

		/* Check if the supplied handle is invalid. */
		if (!bt_gatt_attr_get_handle(params[i].attr)) {
			return -EINVAL;
		}

		/* Check if the characteristic is subscribed. */
		if (IS_ENABLED(CONFIG_BT_GATT_ENFORCE_SUBSCRIPTION) &&
		    !bt_gatt_is_subscribed(conn, params[i].attr,
					   BT_GATT_CCC_NOTIFY)) {
			LOG_WRN("Device is not subscribed to characteristic");
			return -EINVAL;
		}
	}

	/* PDU length is specified with a 16-bit value. */
	if (*total_len > UINT16_MAX) {
		return -ERANGE;
	}

	/* Check there is a bearer with a high enough MTU. */
	if (bt_att_get_mtu(conn) <
	    (sizeof(struct bt_att_notify_mult) + *total_len)) {
		return -ERANGE;
	}

	return 0;
}

int bt_gatt_notify_multiple(struct bt_conn *conn,
			    uint16_t num_params,
			    struct bt_gatt_notify_params params[])
{
	int err;
	size_t total_len = 0;
	struct net_buf *buf;

	/* Validate arguments, connection state and feature support. */
	err = gatt_notify_multiple_verify_args(conn, params, num_params);
	if (err) {
		return err;
	}

	/* Validate all the attributes that we want to notify.
	 * Also gets us the total length of the PDU as a side-effect.
	 */
	err = gatt_notify_multiple_verify_params(conn, params, num_params, &total_len);
	if (err) {
		return err;
	}

	/* Send any outstanding notifications.
	 * Frees up buffer space for our PDU.
	 */
	gatt_notify_flush(conn);

	/* Build the PDU */
	buf = bt_att_create_pdu(conn, BT_ATT_OP_NOTIFY_MULT,
				sizeof(struct bt_att_notify_mult) + total_len);
	if (!buf) {
		return -ENOMEM;
	}

	/* Register the callback. It will be called num_params times. */
	bt_att_set_tx_meta_data(buf, params->func, params->user_data, BT_ATT_CHAN_OPT(params));
	bt_att_increment_tx_meta_data_attr_count(buf, num_params - 1);

	for (uint16_t i = 0; i < num_params; i++) {
		struct notify_data data;

		data.attr = params[i].attr;
		data.handle = bt_gatt_attr_get_handle(data.attr);

		/* Check if attribute is a characteristic then adjust the
		 * handle
		 */
		if (!bt_uuid_cmp(data.attr->uuid, BT_UUID_GATT_CHRC)) {
			data.handle = bt_gatt_attr_value_handle(data.attr);
		}

		/* Add handle and data to the command buffer. */
		gatt_add_nfy_to_buf(buf, data.handle, &params[i]);
	}

	/* Send the buffer. */
	return gatt_notify_mult_send(conn, buf);
}
#endif /* CONFIG_BT_GATT_NOTIFY_MULTIPLE */

int bt_gatt_indicate(struct bt_conn *conn,
		     struct bt_gatt_indicate_params *params)
{
	struct notify_data data;

	__ASSERT(params, "invalid parameters\n");
	__ASSERT(params->attr || params->uuid, "invalid parameters\n");

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		return -EAGAIN;
	}

	if (conn && conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	data.attr = params->attr;
	data.handle = bt_gatt_attr_get_handle(data.attr);

	/* Lookup UUID if it was given */
	if (params->uuid) {
		if (!gatt_find_by_uuid(&data, params->uuid)) {
			return -ENOENT;
		}

		params->attr = data.attr;
	} else {
		if (!data.handle) {
			return -ENOENT;
		}
	}

	/* Check if attribute is a characteristic then adjust the handle */
	if (!bt_uuid_cmp(data.attr->uuid, BT_UUID_GATT_CHRC)) {
		struct bt_gatt_chrc *chrc = data.attr->user_data;

		if (!(chrc->properties & BT_GATT_CHRC_INDICATE)) {
			return -EINVAL;
		}

		data.handle = bt_gatt_attr_value_handle(data.attr);
	}

	if (conn) {
		params->_ref = 1;
		return gatt_indicate(conn, data.handle, params);
	}

	data.err = -ENOTCONN;
	data.type = BT_GATT_CCC_INDICATE;
	data.ind_params = params;

	params->_ref = 0;
	bt_gatt_foreach_attr_type(data.handle, 0xffff, BT_UUID_GATT_CCC, NULL,
				  1, notify_cb, &data);

	return data.err;
}

uint16_t bt_gatt_get_mtu(struct bt_conn *conn)
{
	return bt_att_get_mtu(conn);
}

uint16_t bt_gatt_get_uatt_mtu(struct bt_conn *conn)
{
	return bt_att_get_uatt_mtu(conn);
}

uint8_t bt_gatt_check_perm(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			uint16_t mask)
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

	/*
	 * Core Specification 5.4 Vol. 3 Part C 10.3.1
	 *
	 * If neither an LTK nor an STK is available, the service
	 * request shall be rejected with the error code
	 * “Insufficient Authentication”.
	 * Note: When the link is not encrypted, the error code
	 * “Insufficient Authentication” does not indicate that
	 *  MITM protection is required.
	 *
	 * If an LTK or an STK is available and encryption is
	 * required (LE security mode 1) but encryption is not
	 * enabled, the service request shall be rejected with
	 * the error code “Insufficient Encryption”.
	 */

	if (mask &
	    (BT_GATT_PERM_ENCRYPT_MASK | BT_GATT_PERM_AUTHEN_MASK | BT_GATT_PERM_LESC_MASK)) {
#if defined(CONFIG_BT_SMP)
		if (!conn->encrypt) {
			if (bt_conn_ltk_present(conn)) {
				return BT_ATT_ERR_INSUFFICIENT_ENCRYPTION;
			} else {
				return BT_ATT_ERR_AUTHENTICATION;
			}
		}

		if (mask & BT_GATT_PERM_AUTHEN_MASK) {
			if (bt_conn_get_security(conn) < BT_SECURITY_L3) {
				return BT_ATT_ERR_AUTHENTICATION;
			}
		}

		if (mask & BT_GATT_PERM_LESC_MASK) {
			const struct bt_keys *keys = conn->le.keys;

			if (!keys || (keys->flags & BT_KEYS_SC) == 0) {
				return BT_ATT_ERR_AUTHENTICATION;
			}
		}
#else
		return BT_ATT_ERR_AUTHENTICATION;
#endif /* CONFIG_BT_SMP */
	}

	return 0;
}

static void sc_restore_rsp(struct bt_conn *conn,
			   struct bt_gatt_indicate_params *params, uint8_t err)
{
#if defined(CONFIG_BT_GATT_CACHING)
	struct gatt_cf_cfg *cfg;
#endif

	LOG_DBG("err 0x%02x", err);

#if defined(CONFIG_BT_GATT_CACHING)
	/* BLUETOOTH CORE SPECIFICATION Version 5.3 | Vol 3, Part G page 1476:
	 * 2.5.2.1 Robust Caching
	 * ... a change-unaware connected client using exactly one ATT bearer
	 * becomes change-aware when ...
	 * The client receives and confirms a Handle Value Indication
	 * for the Service Changed characteristic
	 */

	if (bt_att_fixed_chan_only(conn)) {
		cfg = find_cf_cfg(conn);
		if (cfg && CF_ROBUST_CACHING(cfg)) {
			set_change_aware(cfg, true);
		}
	}
#endif /* CONFIG_BT_GATT_CACHING */

	if (!err && IS_ENABLED(CONFIG_BT_GATT_SERVICE_CHANGED)) {
		struct gatt_sc_cfg *gsc_cfg = find_sc_cfg(conn->id, &conn->le.dst);

		if (gsc_cfg) {
			sc_reset(gsc_cfg);
		}
	}
}

static struct bt_gatt_indicate_params sc_restore_params[CONFIG_BT_MAX_CONN];
static uint16_t sc_range[CONFIG_BT_MAX_CONN][2];

static void sc_restore(struct bt_conn *conn)
{
	struct gatt_sc_cfg *cfg;
	uint8_t index;

	cfg = find_sc_cfg(conn->id, &conn->le.dst);
	if (!cfg) {
		LOG_DBG("no SC data found");
		return;
	}

	if (!(cfg->data.start || cfg->data.end)) {
		return;
	}

	LOG_DBG("peer %s start 0x%04x end 0x%04x", bt_addr_le_str(&cfg->peer), cfg->data.start,
		cfg->data.end);

	index = bt_conn_index(conn);

	sc_range[index][0] = sys_cpu_to_le16(cfg->data.start);
	sc_range[index][1] = sys_cpu_to_le16(cfg->data.end);

	sc_restore_params[index].attr = &_1_gatt_svc.attrs[2];
	sc_restore_params[index].func = sc_restore_rsp;
	sc_restore_params[index].data = &sc_range[index][0];
	sc_restore_params[index].len = sizeof(sc_range[index]);

	if (bt_gatt_indicate(conn, &sc_restore_params[index])) {
		LOG_ERR("SC restore indication failed");
	}
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
	struct bt_gatt_ccc_managed_user_data *ccc;
	size_t i;
	uint8_t err;

	if (!is_host_managed_ccc(attr)) {
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
				LOG_WRN("CCC %p not writable", attr);
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
	struct bt_gatt_ccc_managed_user_data *ccc;
	bool value_used;
	size_t i;

	if (!is_host_managed_ccc(attr)) {
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
			if (!bt_le_bond_exists(conn->id, &conn->le.dst)) {
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

		LOG_DBG("ccc %p reset", ccc);
	}

	return BT_GATT_ITER_CONTINUE;
}

bool bt_gatt_is_subscribed(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, uint16_t ccc_type)
{
	uint16_t ccc_bits;
	uint8_t ccc_bits_encoded[sizeof(ccc_bits)];
	ssize_t len;

	__ASSERT(conn, "invalid parameter\n");
	__ASSERT(attr, "invalid parameter\n");

	if (conn->state != BT_CONN_CONNECTED) {
		return false;
	}

	/* Check if attribute is a characteristic declaration */
	if (!bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CHRC)) {
		uint8_t properties;

		if (!attr->read) {
			LOG_ERR("Read method not set");
			return false;
		}
		/* The characterstic properties is the first byte of the attribute value */
		len = attr->read(NULL, attr, &properties, sizeof(properties), 0);
		if (len < 0) {
			LOG_ERR("Failed to read attribute %p (err %zd)", attr, len);
			return false;
		} else if (len != sizeof(properties)) {
			LOG_ERR("Invalid read length: %zd", len);
			return false;
		}

		if (!(properties & (BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_INDICATE))) {
			/* Characteristic doesn't support subscription */
			return false;
		}

		attr = bt_gatt_attr_next(attr);
		__ASSERT(attr, "No more attributes\n");
	}

	/* Check if attribute is a characteristic value */
	if (bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CCC) != 0) {
		attr = bt_gatt_attr_next(attr);
		__ASSERT(attr, "No more attributes\n");
	}

	/* Find the CCC Descriptor */
	while (bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CCC) &&
	       /* Also stop if we leave the current characteristic definition */
	       bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CHRC) &&
	       bt_uuid_cmp(attr->uuid, BT_UUID_GATT_PRIMARY) &&
	       bt_uuid_cmp(attr->uuid, BT_UUID_GATT_SECONDARY)) {
		attr = bt_gatt_attr_next(attr);
		if (!attr) {
			return false;
		}
	}

	if (bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CCC) != 0) {
		return false;
	}

	if (!attr->read) {
		LOG_ERR("Read method not set");
		return false;
	}

	len = attr->read(conn, attr, ccc_bits_encoded, sizeof(ccc_bits_encoded), 0);
	if (len < 0) {
		LOG_ERR("Failed to read attribute %p (err %zd)", attr, len);
		return false;
	} else if (len != sizeof(ccc_bits_encoded)) {
		LOG_ERR("Invalid read length: %zd", len);
		return false;
	}

	ccc_bits = sys_get_le16(ccc_bits_encoded);

	/* Check if the CCC bits match the subscription type */
	if (ccc_bits & ccc_type) {
		return true;
	}

	return false;
}

static bool gatt_sub_is_empty(struct gatt_sub *sub)
{
	return sys_slist_is_empty(&sub->list);
}

/** @brief Free sub for reuse.
 */
static void gatt_sub_free(struct gatt_sub *sub)
{
	__ASSERT_NO_MSG(gatt_sub_is_empty(sub));
	bt_addr_le_copy(&sub->peer, BT_ADDR_LE_ANY);
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

	if (gatt_sub_is_empty(sub)) {
		gatt_sub_free(sub);
	}
}

#if defined(CONFIG_BT_GATT_CLIENT)
static struct gatt_sub *gatt_sub_find(struct bt_conn *conn)
{
	for (int i = 0; i < ARRAY_SIZE(subscriptions); i++) {
		struct gatt_sub *sub = &subscriptions[i];

		if (!conn) {
			if (bt_addr_le_eq(&sub->peer, BT_ADDR_LE_ANY)) {
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

		if (id == sub->id && bt_addr_le_eq(&sub->peer, addr)) {
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

static bool check_subscribe_security_level(struct bt_conn *conn,
					   const struct bt_gatt_subscribe_params *params)
{
#if defined(CONFIG_BT_SMP)
	return conn->sec_level >= params->min_security;
#endif
	return true;
}

static void call_notify_cb_and_maybe_unsubscribe(struct bt_conn *conn, struct gatt_sub *sub,
						 uint16_t handle, const void *data, uint16_t length)
{
	struct bt_gatt_subscribe_params *params, *tmp;
	int err;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&sub->list, params, tmp, node) {
		if (handle != params->value_handle) {
			continue;
		}

		if (check_subscribe_security_level(conn, params)) {
			if (params->notify(conn, params, data, length) == BT_GATT_ITER_STOP) {
				err = bt_gatt_unsubscribe(conn, params);
				if (err != 0) {
					LOG_WRN("Failed to unsubscribe (err %d)", err);
				}
			}
		}
	}
}

void bt_gatt_notification(struct bt_conn *conn, uint16_t handle,
			  const void *data, uint16_t length)
{
	struct gatt_sub *sub;

	LOG_DBG("handle 0x%04x length %u", handle, length);

	sub = gatt_sub_find(conn);
	if (!sub) {
		return;
	}

	call_notify_cb_and_maybe_unsubscribe(conn, sub, handle, data, length);
}

void bt_gatt_mult_notification(struct bt_conn *conn, const void *data,
			       uint16_t length)
{
	const struct bt_att_notify_mult *nfy;
	struct net_buf_simple buf;
	struct gatt_sub *sub;

	LOG_DBG("length %u", length);

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

		LOG_DBG("handle 0x%02x len %u", handle, len);

		if (len > buf.len) {
			LOG_ERR("Invalid data len %u > %u", len, length);
			return;
		}

		call_notify_cb_and_maybe_unsubscribe(conn, sub, handle, nfy->value, len);

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
		atomic_clear_bit(params->flags, BT_GATT_SUBSCRIBE_FLAG_SENT);

		if (!bt_le_bond_exists(conn->id, &conn->le.dst) ||
		    (atomic_test_bit(params->flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE))) {
			/* Remove subscription */
			params->value = 0U;
			gatt_sub_remove(conn, sub, prev, params);
		} else {
			gatt_sub_update(conn, sub);
			prev = &params->node;
		}
	}
}

static void gatt_mtu_rsp(struct bt_conn *conn, int err, const void *pdu,
			 uint16_t length, void *user_data)
{
	struct bt_gatt_exchange_params *params = user_data;

	params->func(conn, att_err_from_int(err), params);
}

static int gatt_exchange_mtu_encode(struct net_buf *buf, size_t len,
				    void *user_data)
{
	struct bt_att_exchange_mtu_req *req;
	uint16_t mtu;

	mtu = BT_LOCAL_ATT_MTU_UATT;

	LOG_DBG("Client MTU %u", mtu);

	req = net_buf_add(buf, sizeof(*req));
	req->mtu = sys_cpu_to_le16(mtu);

	return 0;
}

int bt_gatt_exchange_mtu(struct bt_conn *conn,
			 struct bt_gatt_exchange_params *params)
{
	int err;

	__ASSERT(conn, "invalid parameter\n");
	__ASSERT(params && params->func, "invalid parameters\n");

	if (conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	/* This request shall only be sent once during a connection by the client. */
	if (atomic_test_and_set_bit(conn->flags, BT_CONN_ATT_MTU_EXCHANGED)) {
		return -EALREADY;
	}

	err = gatt_req_send(conn, gatt_mtu_rsp, params,
			    gatt_exchange_mtu_encode, BT_ATT_OP_MTU_REQ,
			    sizeof(struct bt_att_exchange_mtu_req),
			    BT_ATT_CHAN_OPT_UNENHANCED_ONLY);
	if (err) {
		atomic_clear_bit(conn->flags, BT_CONN_ATT_MTU_EXCHANGED);
	}

	return err;
}

static void gatt_discover_next(struct bt_conn *conn, uint16_t last_handle,
			       struct bt_gatt_discover_params *params)
{
	/* Skip if last_handle is not set */
	if (!last_handle) {
		goto discover;
	}

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

static void gatt_find_type_rsp(struct bt_conn *conn, int err,
			       const void *pdu, uint16_t length,
			       void *user_data)
{
	const struct bt_att_handle_group *rsp = pdu;
	struct bt_gatt_discover_params *params = user_data;
	uint8_t count;
	uint16_t end_handle = 0U, start_handle;

	LOG_DBG("err %d", err);

	if (err || (length % sizeof(struct bt_att_handle_group) != 0)) {
		goto done;
	}

	count = length / sizeof(struct bt_att_handle_group);

	/* Parse attributes found */
	for (uint8_t i = 0U; i < count; i++) {
		struct bt_uuid_16 uuid_svc;
		struct bt_gatt_attr attr;
		struct bt_gatt_service_val value;

		start_handle = sys_le16_to_cpu(rsp[i].start_handle);
		end_handle = sys_le16_to_cpu(rsp[i].end_handle);

		LOG_DBG("start_handle 0x%04x end_handle 0x%04x", start_handle, end_handle);

		uuid_svc.uuid.type = BT_UUID_TYPE_16;
		if (params->type == BT_GATT_DISCOVER_PRIMARY) {
			uuid_svc.val = BT_UUID_GATT_PRIMARY_VAL;
		} else {
			uuid_svc.val = BT_UUID_GATT_SECONDARY_VAL;
		}

		value.end_handle = end_handle;
		value.uuid = params->uuid;

		attr = (struct bt_gatt_attr) {
			.uuid = &uuid_svc.uuid,
			.user_data = &value,
			.handle = start_handle,
		};

		if (params->func(conn, &attr, params) == BT_GATT_ITER_STOP) {
			return;
		}
	}

	gatt_discover_next(conn, end_handle, params);

	return;
done:
	params->func(conn, NULL, params);
}

static int gatt_find_type_encode(struct net_buf *buf, size_t len,
				 void *user_data)
{
	struct bt_gatt_discover_params *params = user_data;
	struct bt_att_find_type_req *req;
	uint16_t uuid_val;

	req = net_buf_add(buf, sizeof(*req));
	req->start_handle = sys_cpu_to_le16(params->start_handle);
	req->end_handle = sys_cpu_to_le16(params->end_handle);

	if (params->type == BT_GATT_DISCOVER_PRIMARY) {
		uuid_val = BT_UUID_GATT_PRIMARY_VAL;
	} else {
		uuid_val = BT_UUID_GATT_SECONDARY_VAL;
	}

	req->type = sys_cpu_to_le16(uuid_val);

	LOG_DBG("uuid %s start_handle 0x%04x end_handle 0x%04x", bt_uuid_str(params->uuid),
		params->start_handle, params->end_handle);

	switch (params->uuid->type) {
	case BT_UUID_TYPE_16:
		net_buf_add_le16(buf, BT_UUID_16(params->uuid)->val);
		break;
	case BT_UUID_TYPE_128:
		net_buf_add_mem(buf, BT_UUID_128(params->uuid)->val, 16);
		break;
	}

	return 0;
}

static int gatt_find_type(struct bt_conn *conn,
			  struct bt_gatt_discover_params *params)
{
	size_t len;

	len = sizeof(struct bt_att_find_type_req);

	switch (params->uuid->type) {
	case BT_UUID_TYPE_16:
		len += BT_UUID_SIZE_16;
		break;
	case BT_UUID_TYPE_128:
		len += BT_UUID_SIZE_128;
		break;
	default:
		LOG_ERR("Unknown UUID type %u", params->uuid->type);
		return -EINVAL;
	}

	return gatt_req_send(conn, gatt_find_type_rsp, params,
			     gatt_find_type_encode, BT_ATT_OP_FIND_TYPE_REQ,
			     len, BT_ATT_CHAN_OPT(params));
}

static void read_included_uuid_cb(struct bt_conn *conn, int err,
				  const void *pdu, uint16_t length,
				  void *user_data)
{
	struct bt_gatt_discover_params *params = user_data;
	struct bt_gatt_include value;
	struct bt_gatt_attr attr;
	uint16_t handle;
	union {
		struct bt_uuid uuid;
		struct bt_uuid_128 u128;
	} u;

	if (length != 16U) {
		LOG_ERR("Invalid data len %u", length);
		params->func(conn, NULL, params);
		return;
	}

	handle = params->_included.attr_handle;
	value.start_handle = params->_included.start_handle;
	value.end_handle = params->_included.end_handle;
	value.uuid = &u.uuid;
	u.uuid.type = BT_UUID_TYPE_128;
	memcpy(u.u128.val, pdu, length);

	LOG_DBG("handle 0x%04x uuid %s start_handle 0x%04x "
	       "end_handle 0x%04x\n", params->_included.attr_handle,
	       bt_uuid_str(&u.uuid), value.start_handle, value.end_handle);

	/* Skip if UUID is set but doesn't match */
	if (params->uuid && bt_uuid_cmp(&u.uuid, params->uuid)) {
		goto next;
	}

	attr = (struct bt_gatt_attr) {
		.uuid = BT_UUID_GATT_INCLUDE,
		.user_data = &value,
		.handle = handle,
	};

	if (params->func(conn, &attr, params) == BT_GATT_ITER_STOP) {
		return;
	}
next:
	gatt_discover_next(conn, params->start_handle, params);

	return;
}

static int read_included_uuid_encode(struct net_buf *buf, size_t len,
				     void *user_data)
{
	struct bt_gatt_discover_params *params = user_data;
	struct bt_att_read_req *req;

	req = net_buf_add(buf, sizeof(*req));
	req->handle = sys_cpu_to_le16(params->_included.start_handle);

	return 0;
}

static int read_included_uuid(struct bt_conn *conn,
			      struct bt_gatt_discover_params *params)
{
	LOG_DBG("handle 0x%04x", params->_included.start_handle);

	return gatt_req_send(conn, read_included_uuid_cb, params,
			     read_included_uuid_encode, BT_ATT_OP_READ_REQ,
			     sizeof(struct bt_att_read_req), BT_ATT_CHAN_OPT(params));
}

static uint16_t parse_include(struct bt_conn *conn, const void *pdu,
			   struct bt_gatt_discover_params *params,
			   uint16_t length)
{
	const struct bt_att_read_type_rsp *rsp;
	uint16_t handle = 0U;
	struct bt_gatt_include value;
	union {
		struct bt_uuid uuid;
		struct bt_uuid_16 u16;
		struct bt_uuid_128 u128;
	} u;

	if (length < sizeof(*rsp)) {
		LOG_WRN("Parse err");
		goto done;
	}

	rsp = pdu;

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
		LOG_ERR("Invalid data len %u", rsp->len);
		goto done;
	}

	/* Parse include found */
	for (length--, pdu = rsp->data; length >= rsp->len;
	     length -= rsp->len, pdu = (const uint8_t *)pdu + rsp->len) {
		struct bt_gatt_attr attr;
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

		LOG_DBG("handle 0x%04x uuid %s start_handle 0x%04x "
		       "end_handle 0x%04x\n", handle, bt_uuid_str(&u.uuid),
		       value.start_handle, value.end_handle);

		/* Skip if UUID is set but doesn't match */
		if (params->uuid && bt_uuid_cmp(&u.uuid, params->uuid)) {
			continue;
		}

		attr = (struct bt_gatt_attr) {
			.uuid = BT_UUID_GATT_INCLUDE,
			.user_data = &value,
			.handle = handle,
		};

		if (params->func(conn, &attr, params) == BT_GATT_ITER_STOP) {
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

static uint16_t parse_characteristic(struct bt_conn *conn, const void *pdu,
				  struct bt_gatt_discover_params *params,
				  uint16_t length)
{
	const struct bt_att_read_type_rsp *rsp;
	uint16_t handle = 0U;
	union {
		struct bt_uuid uuid;
		struct bt_uuid_16 u16;
		struct bt_uuid_128 u128;
	} u;

	if (length < sizeof(*rsp)) {
		LOG_WRN("Parse err");
		goto done;
	}

	rsp = pdu;

	/* Data can be either in UUID16 or UUID128 */
	switch (rsp->len) {
	case 7: /* UUID16 */
		u.uuid.type = BT_UUID_TYPE_16;
		break;
	case 21: /* UUID128 */
		u.uuid.type = BT_UUID_TYPE_128;
		break;
	default:
		LOG_ERR("Invalid data len %u", rsp->len);
		goto done;
	}

	/* Parse characteristics found */
	for (length--, pdu = rsp->data; length >= rsp->len;
	     length -= rsp->len, pdu = (const uint8_t *)pdu + rsp->len) {
		struct bt_gatt_attr attr;
		struct bt_gatt_chrc value;
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

		LOG_DBG("handle 0x%04x uuid %s properties 0x%02x", handle, bt_uuid_str(&u.uuid),
			chrc->properties);

		/* Skip if UUID is set but doesn't match */
		if (params->uuid && bt_uuid_cmp(&u.uuid, params->uuid)) {
			continue;
		}

		value = (struct bt_gatt_chrc)BT_GATT_CHRC_INIT(
			&u.uuid, sys_le16_to_cpu(chrc->value_handle),
			chrc->properties);

		attr = (struct bt_gatt_attr) {
			.uuid = BT_UUID_GATT_CHRC,
			.user_data = &value,
			.handle = handle,
		};

		if (params->func(conn, &attr, params) == BT_GATT_ITER_STOP) {
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

static uint16_t parse_read_std_char_desc(struct bt_conn *conn, const void *pdu,
					 struct bt_gatt_discover_params *params,
					 uint16_t length)
{
	const struct bt_att_read_type_rsp *rsp;
	uint16_t handle = 0U;
	uint16_t uuid_val;

	if (params->uuid->type != BT_UUID_TYPE_16) {
		goto done;
	}

	uuid_val = BT_UUID_16(params->uuid)->val;

	if (length < sizeof(*rsp)) {
		LOG_WRN("Parse err");
		goto done;
	}

	rsp = pdu;

	/* Parse characteristics found */
	for (length--, pdu = rsp->data; length >= rsp->len;
	     length -= rsp->len, pdu = (const uint8_t *)pdu + rsp->len) {
		union {
			struct bt_gatt_ccc ccc;
			struct bt_gatt_cpf cpf;
			struct bt_gatt_cep cep;
			struct bt_gatt_scc scc;
		} value;
		const struct bt_att_data *data;
		struct bt_gatt_attr attr;

		if (length < sizeof(*data)) {
			LOG_WRN("Parse err dat");
			goto done;
		}

		data = pdu;

		handle = sys_le16_to_cpu(data->handle);
		/* Handle 0 is invalid */
		if (!handle) {
			goto done;
		}

		switch (uuid_val) {
		case BT_UUID_GATT_CEP_VAL:
			if (length < sizeof(*data) + sizeof(uint16_t)) {
				LOG_WRN("Parse err cep");
				goto done;
			}

			value.cep.properties = sys_get_le16(data->value);
			break;
		case BT_UUID_GATT_CCC_VAL:
			if (length < sizeof(*data) + sizeof(uint16_t)) {
				LOG_WRN("Parse err ccc");
				goto done;
			}

			value.ccc.flags = sys_get_le16(data->value);
			break;
		case BT_UUID_GATT_SCC_VAL:
			if (length < sizeof(*data) + sizeof(uint16_t)) {
				LOG_WRN("Parse err scc");
				goto done;
			}

			value.scc.flags = sys_get_le16(data->value);
			break;
		case BT_UUID_GATT_CPF_VAL:
		{
			struct gatt_cpf *cpf;

			if (length < sizeof(*data) + sizeof(*cpf)) {
				LOG_WRN("Parse err cpf");
				goto done;
			}

			cpf = (void *)data->value;

			value.cpf.format = cpf->format;
			value.cpf.exponent = cpf->exponent;
			value.cpf.unit = sys_le16_to_cpu(cpf->unit);
			value.cpf.name_space = cpf->name_space;
			value.cpf.description = sys_le16_to_cpu(cpf->description);
			break;
		}
		default:
			goto done;
		}

		attr = (struct bt_gatt_attr) {
			.uuid = params->uuid,
			.user_data = &value,
			.handle = handle,
		};

		if (params->func(conn, &attr, params) == BT_GATT_ITER_STOP) {
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

static void gatt_read_type_rsp(struct bt_conn *conn, int err,
			       const void *pdu, uint16_t length,
			       void *user_data)
{
	struct bt_gatt_discover_params *params = user_data;
	uint16_t handle;

	LOG_DBG("err %d", err);

	if (err) {
		params->func(conn, NULL, params);
		return;
	}

	if (params->type == BT_GATT_DISCOVER_INCLUDE) {
		handle = parse_include(conn, pdu, params, length);
	} else if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
		handle = parse_characteristic(conn, pdu, params, length);
	} else {
		handle = parse_read_std_char_desc(conn, pdu, params, length);
	}

	if (!handle) {
		return;
	}

	gatt_discover_next(conn, handle, params);
}

static int gatt_read_type_encode(struct net_buf *buf, size_t len,
				 void *user_data)
{
	struct bt_gatt_discover_params *params = user_data;
	struct bt_att_read_type_req *req;

	req = net_buf_add(buf, sizeof(*req));
	req->start_handle = sys_cpu_to_le16(params->start_handle);
	req->end_handle = sys_cpu_to_le16(params->end_handle);

	switch (params->type) {
	case BT_GATT_DISCOVER_INCLUDE:
		net_buf_add_le16(buf, BT_UUID_GATT_INCLUDE_VAL);
		break;
	case BT_GATT_DISCOVER_CHARACTERISTIC:
		net_buf_add_le16(buf, BT_UUID_GATT_CHRC_VAL);
		break;
	default:
		/* Only 16-bit UUIDs supported */
		net_buf_add_le16(buf, BT_UUID_16(params->uuid)->val);
		break;
	}

	return 0;
}

static int gatt_read_type(struct bt_conn *conn,
			  struct bt_gatt_discover_params *params)
{
	LOG_DBG("start_handle 0x%04x end_handle 0x%04x", params->start_handle, params->end_handle);

	return gatt_req_send(conn, gatt_read_type_rsp, params,
			     gatt_read_type_encode, BT_ATT_OP_READ_TYPE_REQ,
			     sizeof(struct bt_att_read_type_req), BT_ATT_CHAN_OPT(params));
}

static uint16_t parse_service(struct bt_conn *conn, const void *pdu,
				  struct bt_gatt_discover_params *params,
				  uint16_t length)
{
	const struct bt_att_read_group_rsp *rsp;
	uint16_t start_handle, end_handle = 0U;
	union {
		struct bt_uuid uuid;
		struct bt_uuid_16 u16;
		struct bt_uuid_128 u128;
	} u;

	if (length < sizeof(*rsp)) {
		LOG_WRN("Parse err");
		goto done;
	}

	rsp = pdu;

	/* Data can be either in UUID16 or UUID128 */
	switch (rsp->len) {
	case 6: /* UUID16 */
		u.uuid.type = BT_UUID_TYPE_16;
		break;
	case 20: /* UUID128 */
		u.uuid.type = BT_UUID_TYPE_128;
		break;
	default:
		LOG_ERR("Invalid data len %u", rsp->len);
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

		LOG_DBG("start_handle 0x%04x end_handle 0x%04x uuid %s", start_handle, end_handle,
			bt_uuid_str(&u.uuid));

		uuid_svc.uuid.type = BT_UUID_TYPE_16;
		if (params->type == BT_GATT_DISCOVER_PRIMARY) {
			uuid_svc.val = BT_UUID_GATT_PRIMARY_VAL;
		} else {
			uuid_svc.val = BT_UUID_GATT_SECONDARY_VAL;
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

static void gatt_read_group_rsp(struct bt_conn *conn, int err,
				const void *pdu, uint16_t length,
				void *user_data)
{
	struct bt_gatt_discover_params *params = user_data;
	uint16_t handle;

	LOG_DBG("err %d", err);

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

static int gatt_read_group_encode(struct net_buf *buf, size_t len,
				  void *user_data)
{
	struct bt_gatt_discover_params *params = user_data;
	struct bt_att_read_group_req *req;

	req = net_buf_add(buf, sizeof(*req));
	req->start_handle = sys_cpu_to_le16(params->start_handle);
	req->end_handle = sys_cpu_to_le16(params->end_handle);

	if (params->type == BT_GATT_DISCOVER_PRIMARY) {
		net_buf_add_le16(buf, BT_UUID_GATT_PRIMARY_VAL);
	} else {
		net_buf_add_le16(buf, BT_UUID_GATT_SECONDARY_VAL);
	}

	return 0;
}

static int gatt_read_group(struct bt_conn *conn,
			   struct bt_gatt_discover_params *params)
{
	LOG_DBG("start_handle 0x%04x end_handle 0x%04x", params->start_handle, params->end_handle);

	return gatt_req_send(conn, gatt_read_group_rsp, params,
			     gatt_read_group_encode,
			     BT_ATT_OP_READ_GROUP_REQ,
			     sizeof(struct bt_att_read_group_req),
			     BT_ATT_CHAN_OPT(params));
}

static void gatt_find_info_rsp(struct bt_conn *conn, int err,
			       const void *pdu, uint16_t length,
			       void *user_data)
{
	const struct bt_att_find_info_rsp *rsp;
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

	LOG_DBG("err %d", err);

	if (err) {
		goto done;
	}

	if (length < sizeof(*rsp)) {
		LOG_WRN("Parse err");
		goto done;
	}

	rsp = pdu;

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
		LOG_ERR("Invalid format %u", rsp->format);
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
		struct bt_gatt_attr attr;

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

		LOG_DBG("handle 0x%04x uuid %s", handle, bt_uuid_str(&u.uuid));

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

		/* No user_data in this case */
		attr = (struct bt_gatt_attr) {
			.uuid = &u.uuid,
			.handle = handle,
		};

		if (params->func(conn, &attr, params) == BT_GATT_ITER_STOP) {
			return;
		}
	}

	gatt_discover_next(conn, handle, params);

	return;

done:
	params->func(conn, NULL, params);
}

static int gatt_find_info_encode(struct net_buf *buf, size_t len,
				 void *user_data)
{
	struct bt_gatt_discover_params *params = user_data;
	struct bt_att_find_info_req *req;

	req = net_buf_add(buf, sizeof(*req));
	req->start_handle = sys_cpu_to_le16(params->start_handle);
	req->end_handle = sys_cpu_to_le16(params->end_handle);

	return 0;
}

static int gatt_find_info(struct bt_conn *conn,
			  struct bt_gatt_discover_params *params)
{
	LOG_DBG("start_handle 0x%04x end_handle 0x%04x", params->start_handle, params->end_handle);

	return gatt_req_send(conn, gatt_find_info_rsp, params,
			     gatt_find_info_encode, BT_ATT_OP_FIND_INFO_REQ,
			     sizeof(struct bt_att_find_info_req),
			     BT_ATT_CHAN_OPT(params));
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

	case BT_GATT_DISCOVER_STD_CHAR_DESC:
		if (!(params->uuid && params->uuid->type == BT_UUID_TYPE_16 &&
		      (!bt_uuid_cmp(params->uuid, BT_UUID_GATT_CEP) ||
		       !bt_uuid_cmp(params->uuid, BT_UUID_GATT_CCC) ||
		       !bt_uuid_cmp(params->uuid, BT_UUID_GATT_SCC) ||
		       !bt_uuid_cmp(params->uuid, BT_UUID_GATT_CPF)))) {
			return -EINVAL;
		}
		__fallthrough;
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
		LOG_DBG("Invalid discovery type: %u", params->type);
	}

	return -EINVAL;
}

static void parse_read_by_uuid(struct bt_conn *conn,
			       struct bt_gatt_read_params *params,
			       const void *pdu, uint16_t length)
{
	const struct bt_att_read_type_rsp *rsp = pdu;

	const uint16_t req_start_handle = params->by_uuid.start_handle;
	const uint16_t req_end_handle = params->by_uuid.end_handle;

	/* Parse values found */
	for (length--, pdu = rsp->data; length;
	     length -= rsp->len, pdu = (const uint8_t *)pdu + rsp->len) {
		const struct bt_att_data *data = pdu;
		uint16_t handle;
		uint16_t len;

		len = MIN(rsp->len, length);
		if (len < sizeof(struct bt_att_data)) {
			LOG_WRN("Bad peer: ATT read-by-uuid rsp: invalid ATTR PDU len %u", len);
			params->func(conn, BT_ATT_ERR_INVALID_PDU, params, NULL, 0);
			return;
		}

		len -= sizeof(struct bt_att_data);

		handle = sys_le16_to_cpu(data->handle);

		/* Handle 0 is invalid */
		if (!handle) {
			LOG_ERR("Invalid handle");
			return;
		}

		LOG_DBG("handle 0x%04x len %u value %u", handle, rsp->len, len);

		if (!IN_RANGE(handle, req_start_handle, req_end_handle)) {
			LOG_WRN("Bad peer: ATT read-by-uuid rsp: "
				"Handle 0x%04x is outside requested range 0x%04x-0x%04x. "
				"Aborting read.",
				handle, req_start_handle, req_end_handle);
			params->func(conn, BT_ATT_ERR_UNLIKELY, params, NULL, 0);
			return;
		}

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

static void gatt_read_rsp(struct bt_conn *conn, int err, const void *pdu,
			  uint16_t length, void *user_data)
{
	struct bt_gatt_read_params *params = user_data;

	LOG_DBG("err %d", err);

	if (err || !length) {
		params->func(conn, att_err_from_int(err), params, NULL, 0);
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
	 *
	 * Note: Both BT_ATT_OP_READ_RSP and BT_ATT_OP_READ_BLOB_RSP
	 * have an overhead of one octet.
	 */
	if (length < (params->_att_mtu - 1)) {
		params->func(conn, 0, params, NULL, 0);
		return;
	}

	params->single.offset += length;

	/* Continue reading the attribute */
	if (bt_gatt_read(conn, params) < 0) {
		params->func(conn, BT_ATT_ERR_UNLIKELY, params, NULL, 0);
	}
}

static int gatt_read_blob_encode(struct net_buf *buf, size_t len,
				 void *user_data)
{
	struct bt_gatt_read_params *params = user_data;
	struct bt_att_read_blob_req *req;

	req = net_buf_add(buf, sizeof(*req));
	req->handle = sys_cpu_to_le16(params->single.handle);
	req->offset = sys_cpu_to_le16(params->single.offset);

	return 0;
}

static int gatt_read_blob(struct bt_conn *conn,
			  struct bt_gatt_read_params *params)
{
	LOG_DBG("handle 0x%04x offset 0x%04x", params->single.handle, params->single.offset);

	return gatt_req_send(conn, gatt_read_rsp, params,
			     gatt_read_blob_encode, BT_ATT_OP_READ_BLOB_REQ,
			     sizeof(struct bt_att_read_blob_req),
			     BT_ATT_CHAN_OPT(params));
}

static int gatt_read_uuid_encode(struct net_buf *buf, size_t len,
				 void *user_data)
{
	struct bt_gatt_read_params *params = user_data;
	struct bt_att_read_type_req *req;

	req = net_buf_add(buf, sizeof(*req));
	req->start_handle = sys_cpu_to_le16(params->by_uuid.start_handle);
	req->end_handle = sys_cpu_to_le16(params->by_uuid.end_handle);

	if (params->by_uuid.uuid->type == BT_UUID_TYPE_16) {
		net_buf_add_le16(buf, BT_UUID_16(params->by_uuid.uuid)->val);
	} else {
		net_buf_add_mem(buf, BT_UUID_128(params->by_uuid.uuid)->val, 16);
	}

	return 0;
}

static int gatt_read_uuid(struct bt_conn *conn,
			  struct bt_gatt_read_params *params)
{
	LOG_DBG("start_handle 0x%04x end_handle 0x%04x uuid %s", params->by_uuid.start_handle,
		params->by_uuid.end_handle, bt_uuid_str(params->by_uuid.uuid));

	return gatt_req_send(conn, gatt_read_rsp, params,
			     gatt_read_uuid_encode, BT_ATT_OP_READ_TYPE_REQ,
			     sizeof(struct bt_att_read_type_req),
			     BT_ATT_CHAN_OPT(params));
}

#if defined(CONFIG_BT_GATT_READ_MULTIPLE)
static void gatt_read_mult_rsp(struct bt_conn *conn, int err, const void *pdu,
			       uint16_t length, void *user_data)
{
	struct bt_gatt_read_params *params = user_data;

	LOG_DBG("err %d", err);

	if (err || !length) {
		params->func(conn, att_err_from_int(err), params, NULL, 0);
		return;
	}

	params->func(conn, 0, params, pdu, length);

	/* mark read as complete since read multiple is single response */
	params->func(conn, 0, params, NULL, 0);
}

static int gatt_read_mult_encode(struct net_buf *buf, size_t len,
				 void *user_data)
{
	struct bt_gatt_read_params *params = user_data;
	uint8_t i;

	for (i = 0U; i < params->handle_count; i++) {
		net_buf_add_le16(buf, params->multiple.handles[i]);
	}

	return 0;
}

static int gatt_read_mult(struct bt_conn *conn,
			  struct bt_gatt_read_params *params)
{
	LOG_DBG("handle_count %zu", params->handle_count);

	return gatt_req_send(conn, gatt_read_mult_rsp, params,
			     gatt_read_mult_encode, BT_ATT_OP_READ_MULT_REQ,
			     params->handle_count * sizeof(uint16_t),
			     BT_ATT_CHAN_OPT(params));
}

#else
static int gatt_read_mult(struct bt_conn *conn,
			      struct bt_gatt_read_params *params)
{
	return -ENOTSUP;
}
#endif /* CONFIG_BT_GATT_READ_MULTIPLE */

#if defined(CONFIG_BT_GATT_READ_MULT_VAR_LEN)
static void gatt_read_mult_vl_rsp(struct bt_conn *conn, int err,
				  const void *pdu, uint16_t length,
				  void *user_data)
{
	struct bt_gatt_read_params *params = user_data;
	const struct bt_att_read_mult_vl_rsp *rsp;
	struct net_buf_simple buf;

	LOG_DBG("err %d", err);

	if (err || !length) {
		params->func(conn, att_err_from_int(err), params, NULL, 0);
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

static int gatt_read_mult_vl_encode(struct net_buf *buf, size_t len,
				    void *user_data)
{
	struct bt_gatt_read_params *params = user_data;
	uint8_t i;

	for (i = 0U; i < params->handle_count; i++) {
		net_buf_add_le16(buf, params->multiple.handles[i]);
	}

	return 0;
}

static int gatt_read_mult_vl(struct bt_conn *conn,
			     struct bt_gatt_read_params *params)
{
	LOG_DBG("handle_count %zu", params->handle_count);

	return gatt_req_send(conn, gatt_read_mult_vl_rsp, params,
			     gatt_read_mult_vl_encode,
			     BT_ATT_OP_READ_MULT_VL_REQ,
			     params->handle_count * sizeof(uint16_t),
			     BT_ATT_CHAN_OPT(params));
}

#else
static int gatt_read_mult_vl(struct bt_conn *conn,
			     struct bt_gatt_read_params *params)
{
	return -ENOTSUP;
}
#endif /* CONFIG_BT_GATT_READ_MULT_VAR_LEN */

static int gatt_read_encode(struct net_buf *buf, size_t len, void *user_data)
{
	struct bt_gatt_read_params *params = user_data;
	struct bt_att_read_req *req;

	req = net_buf_add(buf, sizeof(*req));
	req->handle = sys_cpu_to_le16(params->single.handle);

	return 0;
}

int bt_gatt_read(struct bt_conn *conn, struct bt_gatt_read_params *params)
{
	__ASSERT(conn, "invalid parameters\n");
	__ASSERT(params && params->func, "invalid parameters\n");

	if (conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	if (params->handle_count == 0) {
		return gatt_read_uuid(conn, params);
	}

	if (params->handle_count > 1) {
		if (params->multiple.variable) {
			return gatt_read_mult_vl(conn, params);
		} else {
			return gatt_read_mult(conn, params);
		}
	}

	if (params->single.offset) {
		return gatt_read_blob(conn, params);
	}

	LOG_DBG("handle 0x%04x", params->single.handle);

	return gatt_req_send(conn, gatt_read_rsp, params, gatt_read_encode,
			     BT_ATT_OP_READ_REQ, sizeof(struct bt_att_read_req),
			     BT_ATT_CHAN_OPT(params));
}

static void gatt_write_rsp(struct bt_conn *conn, int err, const void *pdu,
			   uint16_t length, void *user_data)
{
	struct bt_gatt_write_params *params = user_data;

	LOG_DBG("err %d", err);

	params->func(conn, att_err_from_int(err), params);
}

int bt_gatt_write_without_response_cb(struct bt_conn *conn, uint16_t handle,
				      const void *data, uint16_t length, bool sign,
				      bt_gatt_complete_func_t func,
				      void *user_data)
{
	struct net_buf *buf;
	struct bt_att_write_cmd *cmd;
	__maybe_unused size_t write;

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
	__ASSERT(write == length, "Unable to allocate length %u: only %zu written", length, write);

	LOG_DBG("handle 0x%04x length %u", handle, length);

	bt_att_set_tx_meta_data(buf, func, user_data, BT_ATT_CHAN_OPT_NONE);

	return bt_att_send(conn, buf);
}

static int gatt_exec_encode(struct net_buf *buf, size_t len, void *user_data)
{
	struct bt_att_exec_write_req *req;

	req = net_buf_add(buf, sizeof(*req));
	req->flags = BT_ATT_FLAG_EXEC;

	return 0;
}

static int gatt_exec_write(struct bt_conn *conn,
			   struct bt_gatt_write_params *params)
{
	LOG_DBG("");

	return gatt_req_send(conn, gatt_write_rsp, params, gatt_exec_encode,
			     BT_ATT_OP_EXEC_WRITE_REQ,
			     sizeof(struct bt_att_exec_write_req),
			     BT_ATT_CHAN_OPT(params));
}

static int gatt_cancel_encode(struct net_buf *buf, size_t len, void *user_data)
{
	struct bt_att_exec_write_req *req;

	req = net_buf_add(buf, sizeof(*req));
	req->flags = BT_ATT_FLAG_CANCEL;

	return 0;
}

static int gatt_cancel_all_writes(struct bt_conn *conn,
			   struct bt_gatt_write_params *params)
{
	LOG_DBG("");

	return gatt_req_send(conn, gatt_write_rsp, params, gatt_cancel_encode,
			     BT_ATT_OP_EXEC_WRITE_REQ,
			     sizeof(struct bt_att_exec_write_req),
			     BT_ATT_CHAN_OPT(params));
}

static void gatt_prepare_write_rsp(struct bt_conn *conn, int err,
				   const void *pdu, uint16_t length,
				   void *user_data)
{
	struct bt_gatt_write_params *params = user_data;
	const struct bt_att_prepare_write_rsp *rsp;
	size_t len;
	bool data_valid;

	LOG_DBG("err %d", err);

	/* Don't continue in case of error */
	if (err) {
		params->func(conn, att_err_from_int(err), params);
		return;
	}

	if (length < sizeof(*rsp)) {
		LOG_WRN("Parse err");
		goto fail;
	}

	rsp = pdu;

	len = length - sizeof(*rsp);
	if (len > params->length) {
		LOG_ERR("Incorrect length, canceling write");
		if (gatt_cancel_all_writes(conn, params)) {
			goto fail;
		}

		return;
	}

	data_valid = memcmp(params->data, rsp->value, len) == 0;
	if (params->offset != rsp->offset || !data_valid) {
		LOG_ERR("Incorrect offset or data in response, canceling write");
		if (gatt_cancel_all_writes(conn, params)) {
			goto fail;
		}

		return;
	}

	/* Update params */
	params->offset += len;
	params->data = (const uint8_t *)params->data + len;
	params->length -= len;

	/* If there is no more data execute */
	if (!params->length) {
		if (gatt_exec_write(conn, params)) {
			goto fail;
		}

		return;
	}

	/* Write next chunk */
	if (!bt_gatt_write(conn, params)) {
		/* Success */
		return;
	}

fail:
	/* Notify application that the write operation has failed */
	params->func(conn, BT_ATT_ERR_UNLIKELY, params);
}

static int gatt_prepare_write_encode(struct net_buf *buf, size_t len,
				     void *user_data)
{
	struct bt_gatt_write_params *params = user_data;
	struct bt_att_prepare_write_req *req;
	size_t write;

	req = net_buf_add(buf, sizeof(*req));
	req->handle = sys_cpu_to_le16(params->handle);
	req->offset = sys_cpu_to_le16(params->offset);

	write = net_buf_append_bytes(buf, len - sizeof(*req),
				     (uint8_t *)params->data, K_NO_WAIT, NULL,
				     NULL);
	if (write != (len - sizeof(*req))) {
		return -ENOMEM;
	}

	return 0;
}

static int gatt_prepare_write(struct bt_conn *conn,
			      struct bt_gatt_write_params *params)
{
	uint16_t len, req_len;
	uint16_t mtu = bt_att_get_mtu(conn);

	req_len = sizeof(struct bt_att_prepare_write_req);

	/** MTU size is bigger than the ATT_PREPARE_WRITE_REQ header (5 bytes),
	 * unless there's no connection.
	 */
	if (mtu == 0) {
		return -ENOTCONN;
	}

	len = mtu - req_len - 1;
	len = MIN(params->length, len);
	len += req_len;

	return gatt_req_send(conn, gatt_prepare_write_rsp, params,
			     gatt_prepare_write_encode,
			     BT_ATT_OP_PREPARE_WRITE_REQ, len,
			     BT_ATT_CHAN_OPT(params));
}

static int gatt_write_encode(struct net_buf *buf, size_t len, void *user_data)
{
	struct bt_gatt_write_params *params = user_data;
	struct bt_att_write_req *req;
	size_t write;

	req = net_buf_add(buf, sizeof(*req));
	req->handle = sys_cpu_to_le16(params->handle);

	write = net_buf_append_bytes(buf, params->length, params->data,
				     K_NO_WAIT, NULL, NULL);
	if (write != params->length) {
		return -ENOMEM;
	}

	return 0;
}

int bt_gatt_write(struct bt_conn *conn, struct bt_gatt_write_params *params)
{
	size_t len;

	__ASSERT(conn, "invalid parameters\n");
	__ASSERT(params && params->func, "invalid parameters\n");
	__ASSERT(params->handle, "invalid parameters\n");

	if (conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	len = sizeof(struct bt_att_write_req) + params->length;

	/* Use Prepare Write if offset is set or Long Write is required */
	if (params->offset || len > (bt_att_get_mtu(conn) - 1)) {
		return gatt_prepare_write(conn, params);
	}

	LOG_DBG("handle 0x%04x length %u", params->handle, params->length);

	return gatt_req_send(conn, gatt_write_rsp, params, gatt_write_encode,
			     BT_ATT_OP_WRITE_REQ, len, BT_ATT_CHAN_OPT(params));
}

static void gatt_write_ccc_rsp(struct bt_conn *conn, int err,
			       const void *pdu, uint16_t length,
			       void *user_data)
{
	struct bt_gatt_subscribe_params *params = user_data;
	uint8_t att_err;

	LOG_DBG("err %d", err);

	atomic_clear_bit(params->flags, BT_GATT_SUBSCRIBE_FLAG_WRITE_PENDING);

	/* if write to CCC failed we remove subscription and notify app */
	if (err) {
		struct gatt_sub *sub;
		sys_snode_t *node, *tmp, *prev;

		sub = gatt_sub_find(conn);
		if (!sub) {
			return;
		}

		prev = NULL;

		SYS_SLIST_FOR_EACH_NODE_SAFE(&sub->list, node, tmp) {
			if (node == &params->node) {
				gatt_sub_remove(conn, sub, prev, params);
				break;
			}
			prev = node;
		}
	} else if (!params->value) {
		/* Notify with NULL data to complete unsubscribe */
		params->notify(conn, params, NULL, 0);
	}

	att_err = att_err_from_int(err);

	if (params->subscribe) {
		params->subscribe(conn, att_err, params);
	}
}


static int gatt_write_ccc_buf(struct net_buf *buf, size_t len, void *user_data)
{
	struct bt_gatt_subscribe_params *params = user_data;
	struct bt_att_write_req *write_req;

	write_req = net_buf_add(buf, sizeof(*write_req));
	write_req->handle = sys_cpu_to_le16(params->ccc_handle);
	net_buf_add_le16(buf, params->value);

	atomic_set_bit(params->flags, BT_GATT_SUBSCRIBE_FLAG_WRITE_PENDING);

	return 0;
}

static int gatt_write_ccc(struct bt_conn *conn,
			  struct bt_gatt_subscribe_params *params,
			  bt_att_func_t rsp)
{
	size_t len = sizeof(struct bt_att_write_req) + sizeof(uint16_t);

	LOG_DBG("handle 0x%04x value 0x%04x", params->ccc_handle, params->value);

	/* The value of the params doesn't matter, this is just so we don't
	 * repeat CCC writes when the AUTO_RESUBSCRIBE quirk is enabled.
	 */
	atomic_set_bit(params->flags, BT_GATT_SUBSCRIBE_FLAG_SENT);

	return gatt_req_send(conn, rsp, params,
			     gatt_write_ccc_buf, BT_ATT_OP_WRITE_REQ, len,
			     BT_ATT_CHAN_OPT(params));
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
#if defined(CONFIG_BT_EATT)
	params->disc_params->chan_opt = params->chan_opt;
#endif /* CONFIG_BT_EATT */

	err = bt_gatt_discover(conn, params->disc_params);
	if (err) {
		LOG_DBG("CCC Discovery failed (err %d)", err);
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

#if defined(CONFIG_BT_GATT_AUTO_DISCOVER_CCC)
	if (params->disc_params != NULL && params->disc_params->func == gatt_ccc_discover_cb) {
		/* Already in progress */
		return -EBUSY;
	}
#endif

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
		if (params->ccc_handle == BT_GATT_AUTO_DISCOVER_CCC_HANDLE) {
			return gatt_ccc_discover(conn, params);
		}
#endif
		err = gatt_write_ccc(conn, params, gatt_write_ccc_rsp);
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
	struct bt_gatt_subscribe_params *tmp;
	bool has_subscription = false, found = false;

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
	SYS_SLIST_FOR_EACH_CONTAINER(&sub->list, tmp, node) {
		if (params == tmp) {
			found = true;
			continue;
		}

		/* Check if there still remains any other subscription */
		if (tmp->value_handle == params->value_handle) {
			has_subscription = true;
		}
	}

	if (!found) {
		return -EINVAL;
	}

	/* Attempt to cancel if write is pending */
	if (atomic_test_bit(params->flags, BT_GATT_SUBSCRIBE_FLAG_WRITE_PENDING)) {
		bt_gatt_cancel(conn, params);
	}

	if (!has_subscription) {
		int err;

		params->value = 0x0000;
		err = gatt_write_ccc(conn, params, gatt_write_ccc_rsp);
		if (err) {
			return err;
		}
	}

	sys_slist_find_and_remove(&sub->list, &params->node);

	if (gatt_sub_is_empty(sub)) {
		gatt_sub_free(sub);
	}

	if (has_subscription) {
		/* Notify with NULL data to complete unsubscribe */
		params->notify(conn, params, NULL, 0);
	}

	return 0;
}

void bt_gatt_cancel(struct bt_conn *conn, void *params)
{
	struct bt_att_req *req;
	bt_att_func_t func = NULL;

	k_sched_lock();

	req = bt_att_find_req_by_user_data(conn, params);
	if (req) {
		func = req->func;
		bt_att_req_cancel(conn, req);
	}

	k_sched_unlock();

	if (func) {
		func(conn, BT_ATT_ERR_UNLIKELY, NULL, 0, params);
	}
}

#if defined(CONFIG_BT_GATT_AUTO_RESUBSCRIBE)
static void gatt_resub_ccc_rsp(struct bt_conn *conn, int err,
			       const void *pdu, uint16_t length,
			       void *user_data)
{
	LOG_DBG("err %d", err);

	if (err == -ECONNRESET) {
		/* The resubscriptions are implicit, thus in the case of ACL
		 * disconnection during the CCC value ATT Write, there is no
		 * need to notify the application.
		 */
		return;
	}

	gatt_write_ccc_rsp(conn, err, pdu, length, user_data);
}

static int gatt_resub_ccc(struct bt_conn *conn,
			  struct bt_gatt_subscribe_params *params)
{
	return gatt_write_ccc(conn, params, gatt_resub_ccc_rsp);
}

static void add_subscriptions(struct bt_conn *conn)
{
	struct gatt_sub *sub;
	struct bt_gatt_subscribe_params *params;

	if (!bt_le_bond_exists(conn->id, &conn->le.dst)) {
		return;
	}

	sub = gatt_sub_find(conn);
	if (!sub) {
		return;
	}

	/* Lookup existing subscriptions */
	SYS_SLIST_FOR_EACH_CONTAINER(&sub->list, params, node) {
		if (!atomic_test_bit(params->flags,
				     BT_GATT_SUBSCRIBE_FLAG_SENT) &&
		    !atomic_test_bit(params->flags,
				     BT_GATT_SUBSCRIBE_FLAG_NO_RESUB)) {
			int err;

			/* Force write to CCC to workaround devices that don't
			 * track it properly.
			 */
			err = gatt_resub_ccc(conn, params);
			if (err < 0) {
				LOG_WRN("conn %p params %p resub failed (err %d)",
					(void *)conn, params, err);
			}
		}
	}
}
#endif	/* CONFIG_BT_GATT_AUTO_RESUBSCRIBE */

#if defined(CONFIG_BT_GATT_AUTO_UPDATE_MTU)
static void gatt_exchange_mtu_func(struct bt_conn *conn, uint8_t err,
				   struct bt_gatt_exchange_params *params)
{
	if (err) {
		LOG_WRN("conn %p err 0x%02x", conn, err);
	}
}

static struct bt_gatt_exchange_params gatt_exchange_params = {
	.func = gatt_exchange_mtu_func,
};
#endif /* CONFIG_BT_GATT_AUTO_UPDATE_MTU */
#endif /* CONFIG_BT_GATT_CLIENT */

#if defined(CONFIG_BT_SETTINGS_CCC_STORE_MAX)
#define CCC_STORE_MAX CONFIG_BT_SETTINGS_CCC_STORE_MAX
#else /* defined(CONFIG_BT_SETTINGS_CCC_STORE_MAX) */
#define CCC_STORE_MAX 0
#endif /* defined(CONFIG_BT_SETTINGS_CCC_STORE_MAX) */

static struct bt_gatt_ccc_cfg *ccc_find_cfg(struct bt_gatt_ccc_managed_user_data *ccc,
					    const bt_addr_le_t *addr,
					    uint8_t id)
{
	for (size_t i = 0; i < ARRAY_SIZE(ccc->cfg); i++) {
		if (id == ccc->cfg[i].id &&
		    bt_addr_le_eq(&ccc->cfg[i].peer, addr)) {
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

static void ccc_clear(struct bt_gatt_ccc_managed_user_data *ccc,
		      const bt_addr_le_t *addr,
		      uint8_t id)
{
	struct bt_gatt_ccc_cfg *cfg;

	cfg = ccc_find_cfg(ccc, addr, id);
	if (!cfg) {
		LOG_DBG("Unable to clear CCC: cfg not found");
		return;
	}

	clear_ccc_cfg(cfg);
}

static uint8_t ccc_load(const struct bt_gatt_attr *attr, uint16_t handle,
			void *user_data)
{
	struct ccc_load *load = user_data;
	struct bt_gatt_ccc_managed_user_data *ccc;
	struct bt_gatt_ccc_cfg *cfg;

	if (!is_host_managed_ccc(attr)) {
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
			LOG_DBG("Unable to restore CCC: handle 0x%04x cannot be"
			       " found",  load->entry->handle);
			goto next;
		}
		return BT_GATT_ITER_CONTINUE;
	}

	LOG_DBG("Restoring CCC: handle 0x%04x value 0x%04x", load->entry->handle,
		load->entry->value);

	cfg = ccc_find_cfg(ccc, load->addr_with_id.addr, load->addr_with_id.id);
	if (!cfg) {
		cfg = ccc_find_cfg(ccc, BT_ADDR_LE_ANY, 0);
		if (!cfg) {
			LOG_DBG("Unable to restore CCC: no cfg left");
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
	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		struct ccc_store ccc_store[CCC_STORE_MAX];
		struct ccc_load load;
		bt_addr_le_t addr;
		ssize_t len;
		int err;
		const char *next;

		settings_name_next(name, &next);

		if (!name) {
			LOG_ERR("Insufficient number of arguments");
			return -EINVAL;
		} else if (!next) {
			load.addr_with_id.id = BT_ID_DEFAULT;
		} else {
			unsigned long next_id = strtoul(next, NULL, 10);

			if (next_id >= CONFIG_BT_ID_MAX) {
				LOG_ERR("Invalid local identity %lu", next_id);
				return -EINVAL;
			}

			load.addr_with_id.id = (uint8_t)next_id;
		}

		err = bt_settings_decode_key(name, &addr);
		if (err) {
			LOG_ERR("Unable to decode address %s", name);
			return -EINVAL;
		}

		load.addr_with_id.addr = &addr;

		if (len_rd) {
			len = read_cb(cb_arg, ccc_store, sizeof(ccc_store));

			if (len < 0) {
				LOG_ERR("Failed to decode value (err %zd)", len);
				return len;
			}

			load.entry = ccc_store;
			load.count = len / sizeof(*ccc_store);

			for (size_t i = 0; i < load.count; i++) {
				LOG_DBG("Read CCC: handle 0x%04x value 0x%04x", ccc_store[i].handle,
					ccc_store[i].value);
			}
		} else {
			load.entry = NULL;
			load.count = 0;
		}

		bt_gatt_foreach_attr(0x0001, 0xffff, ccc_load, &load);

		LOG_DBG("Restored CCC for id:%" PRIu8 " addr:%s", load.addr_with_id.id,
			bt_addr_le_str(load.addr_with_id.addr));
	}

	return 0;
}

#ifdef CONFIG_BT_SETTINGS
static int ccc_set_cb(const char *name, size_t len_rd, settings_read_cb read_cb,
		      void *cb_arg)
{
	if (IS_ENABLED(CONFIG_BT_SETTINGS_CCC_LAZY_LOADING)) {
		/* Only load CCCs on demand */
		return 0;
	}

	return ccc_set(name, len_rd, read_cb, cb_arg);
}

BT_SETTINGS_DEFINE(ccc, "ccc", ccc_set_cb, NULL);
#endif /* CONFIG_BT_SETTINGS */

static int ccc_set_direct(const char *key, size_t len, settings_read_cb read_cb,
			  void *cb_arg, void *param)
{
	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		const char *name;

		LOG_DBG("key: %s", (const char *)param);

		/* Only "bt/ccc" settings should ever come here */
		if (!settings_name_steq((const char *)param, "bt/ccc", &name)) {
			LOG_ERR("Invalid key");
			return -EINVAL;
		}

		return ccc_set(name, len, read_cb, cb_arg);
	}
	return 0;
}

void bt_gatt_connected(struct bt_conn *conn)
{
	struct conn_data data;

	LOG_DBG("conn %p", conn);

	data.conn = conn;
	data.sec = BT_SECURITY_L1;

	/* Load CCC settings from backend if bonded */
	if (IS_ENABLED(CONFIG_BT_SETTINGS_CCC_LAZY_LOADING) &&
	    bt_le_bond_exists(conn->id, &conn->le.dst)) {
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
	    (conn->role == BT_HCI_ROLE_CENTRAL ||
	     IS_ENABLED(CONFIG_BT_GATT_AUTO_SEC_REQ)) &&
	    bt_conn_get_security(conn) < data.sec) {
		int err = bt_conn_set_security(conn, data.sec);

		if (err) {
			LOG_WRN("Failed to set security for bonded peer (%d)", err);
		}
	}

#if defined(CONFIG_BT_GATT_AUTO_UPDATE_MTU)
	int err;

	err = bt_gatt_exchange_mtu(conn, &gatt_exchange_params);
	if (err) {
		LOG_WRN("MTU Exchange failed (err %d)", err);
	}
#endif /* CONFIG_BT_GATT_AUTO_UPDATE_MTU */
}

void bt_gatt_att_max_mtu_changed(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	struct bt_gatt_cb *cb;

	SYS_SLIST_FOR_EACH_CONTAINER(&callback_list, cb, node) {
		if (cb->att_mtu_updated) {
			cb->att_mtu_updated(conn, tx, rx);
		}
	}
}

void bt_gatt_encrypt_change(struct bt_conn *conn)
{
	struct conn_data data;

	LOG_DBG("conn %p", conn);

	data.conn = conn;
	data.sec = BT_SECURITY_L1;

#if defined(CONFIG_BT_GATT_AUTO_RESUBSCRIBE)
	add_subscriptions(conn);
#endif	/* CONFIG_BT_GATT_AUTO_RESUBSCRIBE */

	bt_gatt_foreach_attr(0x0001, 0xffff, update_ccc, &data);

	if (!bt_gatt_change_aware(conn, false)) {
		/* Send a Service Changed indication if the current peer is
		 * marked as change-unaware.
		 */
		sc_indicate(0x0001, 0xffff);
	}
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

	/* BLUETOOTH CORE SPECIFICATION Version 5.3 | Vol 3, Part G page 1475:
	 * 2.5.2.1 Robust Caching
	 * A change-unaware connected client becomes change-aware when it reads
	 * the Database Hash characteristic and then the server receives another
	 * ATT request from the client.
	 */
	if (atomic_test_and_clear_bit(cfg->flags, CF_DB_HASH_READ)) {
		bt_att_clear_out_of_sync_sent(conn);
		set_change_aware(cfg, true);
		return true;
	}

	/* BLUETOOTH CORE SPECIFICATION Version 5.3 | Vol 3, Part G page 1476:
	 * 2.5.2.1 Robust Caching
	 * ... a change-unaware connected client using exactly one ATT bearer
	 * becomes change-aware when ...
	 * The server sends the client a response with the Error Code parameter
	 * set to Database Out Of Sync (0x12) and then the server receives
	 * another ATT request from the client.
	 */
	if (bt_att_fixed_chan_only(conn) && bt_att_out_of_sync_sent_on_fixed(conn)) {
		atomic_clear_bit(cfg->flags, CF_DB_HASH_READ);
		bt_att_clear_out_of_sync_sent(conn);
		set_change_aware(cfg, true);
		return true;
	}

	return false;
#else
	return true;
#endif
}

static struct gatt_cf_cfg *find_cf_cfg_by_addr(uint8_t id,
					       const bt_addr_le_t *addr)
{
	if (IS_ENABLED(CONFIG_BT_GATT_CACHING)) {
		int i;

		for (i = 0; i < ARRAY_SIZE(cf_cfg); i++) {
			if (id == cf_cfg[i].id &&
			    bt_addr_le_eq(addr, &cf_cfg[i].peer)) {
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
	struct bt_gatt_ccc_managed_user_data *ccc;
	struct bt_gatt_ccc_cfg *cfg;

	if (!is_host_managed_ccc(attr)) {
		return BT_GATT_ITER_CONTINUE;
	}

	ccc = attr->user_data;

	/* Check if there is a cfg for the peer */
	cfg = ccc_find_cfg(ccc, save->addr_with_id.addr, save->addr_with_id.id);
	if (!cfg) {
		return BT_GATT_ITER_CONTINUE;
	}

	LOG_DBG("Storing CCCs handle 0x%04x value 0x%04x", handle, cfg->value);

	CHECKIF(save->count >= CCC_STORE_MAX) {
		LOG_ERR("Too many Client Characteristic Configuration. "
				"See CONFIG_BT_SETTINGS_CCC_STORE_MAX\n");
		return BT_GATT_ITER_STOP;
	}

	save->store[save->count].handle = handle;
	save->store[save->count].value = cfg->value;
	save->count++;

	return BT_GATT_ITER_CONTINUE;
}

static int gatt_store_ccc(uint8_t id, const bt_addr_le_t *addr)
{
	struct ccc_save save;
	size_t len;
	char *str;
	int err;

	save.addr_with_id.addr = addr;
	save.addr_with_id.id = id;
	save.count = 0;

	bt_gatt_foreach_attr(0x0001, 0xffff, ccc_save, &save);

	if (save.count) {
		str = (char *)save.store;
		len = save.count * sizeof(*save.store);
	} else {
		/* No entries to encode, just clear */
		str = NULL;
		len = 0;
	}

	err = bt_settings_store_ccc(id, addr, str, len);
	if (err) {
		LOG_ERR("Failed to store CCCs (err %d)", err);
		return err;
	}

	LOG_DBG("Stored CCCs for %s", bt_addr_le_str(addr));
	if (len) {
		for (size_t i = 0; i < save.count; i++) {
			LOG_DBG("  CCC: handle 0x%04x value 0x%04x", save.store[i].handle,
				save.store[i].value);
		}
	} else {
		LOG_DBG("  CCC: NULL");
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
		LOG_ERR("Insufficient number of arguments");
		return -EINVAL;
	}

	err = bt_settings_decode_key(name, &addr);
	if (err) {
		LOG_ERR("Unable to decode address %s", name);
		return -EINVAL;
	}

	settings_name_next(name, &next);

	if (!next) {
		id = BT_ID_DEFAULT;
	} else {
		unsigned long next_id = strtoul(next, NULL, 10);

		if (next_id >= CONFIG_BT_ID_MAX) {
			LOG_ERR("Invalid local identity %lu", next_id);
			return -EINVAL;
		}

		id = (uint8_t)next_id;
	}

	cfg = find_sc_cfg(id, &addr);
	if (!cfg && len_rd) {
		/* Find and initialize a free sc_cfg entry */
		cfg = find_sc_cfg(BT_ID_DEFAULT, BT_ADDR_LE_ANY);
		if (!cfg) {
			LOG_ERR("Unable to restore SC: no cfg left");
			return -ENOMEM;
		}

		cfg->id = id;
		bt_addr_le_copy(&cfg->peer, &addr);
	}

	if (len_rd) {
		len = read_cb(cb_arg, &cfg->data, sizeof(cfg->data));
		if (len < 0) {
			LOG_ERR("Failed to decode value (err %zd)", len);
			return len;
		}

		LOG_DBG("Read SC: len %zd", len);

		LOG_DBG("Restored SC for %s", bt_addr_le_str(&addr));
	} else if (cfg) {
		/* Clear configuration */
		memset(cfg, 0, sizeof(*cfg));

		LOG_DBG("Removed SC for %s", bt_addr_le_str(&addr));
	}

	return 0;
}

static int sc_commit(void)
{
	atomic_set_bit(gatt_sc.flags, SC_LOAD);
	atomic_clear_bit(gatt_sc.flags, SC_INDICATE_PENDING);

	if (atomic_test_bit(gatt_sc.flags, SC_RANGE_CHANGED)) {
		/* Schedule SC indication since the range has changed */
		sc_work_submit(SC_TIMEOUT);
	}

	return 0;
}

BT_SETTINGS_DEFINE(sc, "sc", sc_set, sc_commit);
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
		LOG_ERR("Insufficient number of arguments");
		return -EINVAL;
	}

	err = bt_settings_decode_key(name, &addr);
	if (err) {
		LOG_ERR("Unable to decode address %s", name);
		return -EINVAL;
	}

	settings_name_next(name, &next);

	if (!next) {
		id = BT_ID_DEFAULT;
	} else {
		unsigned long next_id = strtoul(next, NULL, 10);

		if (next_id >= CONFIG_BT_ID_MAX) {
			LOG_ERR("Invalid local identity %lu", next_id);
			return -EINVAL;
		}

		id = (uint8_t)next_id;
	}

	cfg = find_cf_cfg_by_addr(id, &addr);
	if (!cfg) {
		cfg = find_cf_cfg(NULL);
		if (!cfg) {
			LOG_ERR("Unable to restore CF: no cfg left");
			return -ENOMEM;
		}

		cfg->id = id;
		bt_addr_le_copy(&cfg->peer, &addr);
	}

	if (len_rd) {
		char dst[CF_NUM_BYTES + CF_FLAGS_STORE_LEN];

		len = read_cb(cb_arg, dst, sizeof(dst));
		if (len < 0) {
			LOG_ERR("Failed to decode value (err %zd)", len);
			return len;
		}

		memcpy(cfg->data, dst, sizeof(cfg->data));
		LOG_DBG("Read CF: len %zd", len);

		if (len != sizeof(dst)) {
			LOG_WRN("Change-aware status not found in settings, "
				"defaulting peer status to change-unaware");
			set_change_aware(cfg, false);
		} else {
			/* change-aware byte is present in NVS */
			uint8_t change_aware = dst[sizeof(cfg->data)];

			if (change_aware & ~BIT(CF_CHANGE_AWARE)) {
				LOG_WRN("Read back bad change-aware value: 0x%x, "
					"defaulting peer status to change-unaware",
					change_aware);
				set_change_aware(cfg, false);
			} else {
				set_change_aware_no_store(cfg, change_aware);
			}
		}
	} else {
		clear_cf_cfg(cfg);
	}

	LOG_DBG("Restored CF for %s", bt_addr_le_str(&addr));

	return 0;
}

BT_SETTINGS_DEFINE(cf, "cf", cf_set, NULL);

static int db_hash_set(const char *name, size_t len_rd,
		       settings_read_cb read_cb, void *cb_arg)
{
	ssize_t len;

	len = read_cb(cb_arg, db_hash.stored_hash, sizeof(db_hash.stored_hash));
	if (len < 0) {
		LOG_ERR("Failed to decode value (err %zd)", len);
		return len;
	}

	LOG_HEXDUMP_DBG(db_hash.stored_hash, sizeof(db_hash.stored_hash), "Stored Hash: ");

	return 0;
}

static int db_hash_commit(void)
{
	atomic_set_bit(gatt_sc.flags, DB_HASH_LOAD);

	/* Calculate the hash and compare it against the value loaded from
	 * flash. Do it from the current context to avoid any potential race
	 * conditions.
	 */
	do_db_hash();

	return 0;
}

BT_SETTINGS_DEFINE(hash, "hash", db_hash_set, db_hash_commit);
#endif /*CONFIG_BT_GATT_CACHING */
#endif /* CONFIG_BT_SETTINGS */

static uint8_t remove_peer_from_attr(const struct bt_gatt_attr *attr,
				     uint16_t handle, void *user_data)
{
	const struct addr_with_id *addr_with_id = user_data;
	struct bt_gatt_ccc_managed_user_data *ccc;
	struct bt_gatt_ccc_cfg *cfg;

	if (!is_host_managed_ccc(attr)) {
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
		return bt_settings_delete_ccc(id, addr);
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
		return bt_settings_delete_cf(id, addr);
	}

	return 0;

}


static struct gatt_sub *find_gatt_sub(uint8_t id, const bt_addr_le_t *addr)
{
	for (int i = 0; i < ARRAY_SIZE(subscriptions); i++) {
		struct gatt_sub *sub = &subscriptions[i];

		if (id == sub->id &&
		    bt_addr_le_eq(addr, &sub->peer)) {
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
	LOG_DBG("conn %p", conn);
	bt_gatt_foreach_attr(0x0001, 0xffff, disconnected_cb, conn);

#if defined(CONFIG_BT_GATT_NOTIFY_MULTIPLE)
	/* Clear pending notifications */
	cleanup_notify(conn);
#endif /* CONFIG_BT_GATT_NOTIFY_MULTIPLE */

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		gatt_store_ccc_cf(conn->id, &conn->le.dst);
	}

	/* Make sure to clear the CCC entry when using lazy loading */
	if (IS_ENABLED(CONFIG_BT_SETTINGS_CCC_LAZY_LOADING) &&
	    bt_le_bond_exists(conn->id, &conn->le.dst)) {
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

void bt_gatt_req_set_mtu(struct bt_att_req *req, uint16_t mtu)
{
	IF_ENABLED(CONFIG_BT_GATT_CLIENT, ({
		if (req->func == gatt_read_rsp) {
			struct bt_gatt_read_params *params = req->user_data;

			__ASSERT_NO_MSG(params);
			params->_att_mtu = mtu;
			return;
		}
	}));

	/* Otherwise: This request type does not have an `_att_mtu`
	 * params field or any other method to get this value, so we can
	 * just drop it here. Feel free to add this capability to other
	 * request types if needed.
	 */
}

/* Descriptor of application-specific authorization callbacks that are used
 * with the CONFIG_BT_GATT_AUTHORIZATION_CUSTOM Kconfig enabled.
 */
static const struct bt_gatt_authorization_cb *authorization_cb;

bool bt_gatt_attr_read_authorize(struct bt_conn *conn, const struct bt_gatt_attr *attr)
{
	if (!IS_ENABLED(CONFIG_BT_GATT_AUTHORIZATION_CUSTOM)) {
		return true;
	}

	if (!authorization_cb || !authorization_cb->read_authorize) {
		return true;
	}

	return authorization_cb->read_authorize(conn, attr);
}

bool bt_gatt_attr_write_authorize(struct bt_conn *conn, const struct bt_gatt_attr *attr)
{
	if (!IS_ENABLED(CONFIG_BT_GATT_AUTHORIZATION_CUSTOM)) {
		return true;
	}

	if (!authorization_cb || !authorization_cb->write_authorize) {
		return true;
	}

	return authorization_cb->write_authorize(conn, attr);
}

int bt_gatt_authorization_cb_register(const struct bt_gatt_authorization_cb *cb)
{
	if (!IS_ENABLED(CONFIG_BT_GATT_AUTHORIZATION_CUSTOM)) {
		return -ENOSYS;
	}

	if (!cb) {
		authorization_cb = NULL;
		return 0;
	}

	if (authorization_cb) {
		return -EALREADY;
	}

	authorization_cb = cb;

	return 0;
}
