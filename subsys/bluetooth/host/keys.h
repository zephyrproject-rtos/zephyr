/* keys.h - Bluetooth key handling */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

enum {
	BT_KEYS_SLAVE_LTK      = BIT(0),
	BT_KEYS_IRK            = BIT(1),
	BT_KEYS_LTK            = BIT(2),
	BT_KEYS_LOCAL_CSRK     = BIT(3),
	BT_KEYS_REMOTE_CSRK    = BIT(4),
	BT_KEYS_LTK_P256       = BIT(5),

	BT_KEYS_ALL            = (BT_KEYS_SLAVE_LTK | BT_KEYS_IRK | \
				  BT_KEYS_LTK | BT_KEYS_LOCAL_CSRK | \
				  BT_KEYS_REMOTE_CSRK | BT_KEYS_LTK_P256),
};

enum {
	BT_KEYS_AUTHENTICATED   = BIT(0),
	BT_KEYS_DEBUG           = BIT(1),
	BT_KEYS_ID_PENDING_ADD  = BIT(2),
	BT_KEYS_ID_PENDING_DEL  = BIT(3),
	BT_KEYS_SC              = BIT(4),
};

struct bt_ltk {
	u8_t			rand[8];
	u8_t			ediv[2];
	u8_t			val[16];
};

struct bt_irk {
	u8_t			val[16];
	bt_addr_t		rpa;
};

struct bt_csrk {
	u8_t			val[16];
	u32_t			cnt;
};

struct bt_keys {
	u8_t                    id;
	bt_addr_le_t		addr;
	u8_t                    storage_start[0];
	u8_t			enc_size;
	u8_t                    flags;
	u16_t			keys;
	struct bt_ltk		ltk;
	struct bt_irk		irk;
#if defined(CONFIG_BT_SIGNING)
	struct bt_csrk		local_csrk;
	struct bt_csrk		remote_csrk;
#endif /* BT_SIGNING */
#if !defined(CONFIG_BT_SMP_SC_PAIR_ONLY)
	struct bt_ltk		slave_ltk;
#endif /* CONFIG_BT_SMP_SC_PAIR_ONLY */
#if (defined(CONFIG_BT_KEYS_OVERWRITE_OLDEST))
	u32_t			aging_counter;
#endif /* CONFIG_BT_KEYS_OVERWRITE_OLDEST */
};

#define BT_KEYS_STORAGE_LEN     (sizeof(struct bt_keys) - \
				 offsetof(struct bt_keys, storage_start))

void bt_keys_foreach(int type, void (*func)(struct bt_keys *keys, void *data),
		     void *data);

struct bt_keys *bt_keys_get_addr(u8_t id, const bt_addr_le_t *addr);
struct bt_keys *bt_keys_get_type(int type, u8_t id, const bt_addr_le_t *addr);
struct bt_keys *bt_keys_find(int type, u8_t id, const bt_addr_le_t *addr);
struct bt_keys *bt_keys_find_irk(u8_t id, const bt_addr_le_t *addr);
struct bt_keys *bt_keys_find_addr(u8_t id, const bt_addr_le_t *addr);

void bt_keys_add_type(struct bt_keys *keys, int type);
void bt_keys_clear(struct bt_keys *keys);
void bt_keys_clear_all(u8_t id);

#if defined(CONFIG_BT_SETTINGS)
int bt_keys_store(struct bt_keys *keys);
#else
static inline int bt_keys_store(struct bt_keys *keys)
{
	return 0;
}
#endif

enum {
	BT_LINK_KEY_AUTHENTICATED  = BIT(0),
	BT_LINK_KEY_DEBUG          = BIT(1),
	BT_LINK_KEY_SC             = BIT(2),
};

struct bt_keys_link_key {
	bt_addr_t		addr;
	u8_t                    flags;
	u8_t			val[16];
};

struct bt_keys_link_key *bt_keys_get_link_key(const bt_addr_t *addr);
struct bt_keys_link_key *bt_keys_find_link_key(const bt_addr_t *addr);
void bt_keys_link_key_clear(struct bt_keys_link_key *link_key);
void bt_keys_link_key_clear_addr(const bt_addr_t *addr);

/* This function is used to signal that the key has been used for paring */
/* It updates the aging counter and saves it to flash if configuration option */
/* BT_KEYS_SAVE_AGING_COUNTER_ON_PAIRING is enabled */
void bt_keys_update_usage(u8_t id, const bt_addr_le_t *addr);
