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
	BT_KEYS_AUTHENTICATED,
	BT_KEYS_DEBUG,
	BT_KEYS_ID_PENDING_ADD,
	BT_KEYS_ID_PENDING_DEL,

	/* Total number of flags - must be at the end of the enum */
	BT_KEYS_NUM_FLAGS,
};

struct bt_ltk {
	u64_t			rand;
	u16_t			ediv;
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
	bt_addr_le_t		addr;
	u8_t			enc_size;
	ATOMIC_DEFINE(flags, BT_KEYS_NUM_FLAGS);
	u16_t			keys;
	struct bt_ltk		ltk;
	struct bt_irk		irk;
#if defined(CONFIG_BT_SIGNING)
	struct bt_csrk		local_csrk;
	struct bt_csrk		remote_csrk;
#endif /* BT_SIGNING */
#if !defined(CONFIG_BT_SMP_SC_ONLY)
	struct bt_ltk		slave_ltk;
#endif /* CONFIG_BT_SMP_SC_ONLY */
};

void bt_keys_foreach(int type, void (*func)(struct bt_keys *keys));

struct bt_keys *bt_keys_get_addr(const bt_addr_le_t *addr);
struct bt_keys *bt_keys_get_type(int type, const bt_addr_le_t *addr);
struct bt_keys *bt_keys_find(int type, const bt_addr_le_t *addr);
struct bt_keys *bt_keys_find_irk(const bt_addr_le_t *addr);
struct bt_keys *bt_keys_find_addr(const bt_addr_le_t *addr);

void bt_keys_add_type(struct bt_keys *keys, int type);
void bt_keys_clear(struct bt_keys *keys);
void bt_keys_clear_all(void);

enum {
	BT_LINK_KEY_AUTHENTICATED,
	BT_LINK_KEY_DEBUG,
	BT_LINK_KEY_SC,

	/* Total number of flags - must be at the end of the enum */
	BT_LINK_KEY_NUM_FLAGS,
};

struct bt_keys_link_key {
	bt_addr_t		addr;
	ATOMIC_DEFINE(flags, BT_LINK_KEY_NUM_FLAGS);
	u8_t			val[16];
};

struct bt_keys_link_key *bt_keys_get_link_key(const bt_addr_t *addr);
struct bt_keys_link_key *bt_keys_find_link_key(const bt_addr_t *addr);
void bt_keys_link_key_clear(struct bt_keys_link_key *link_key);
void bt_keys_link_key_clear_addr(const bt_addr_t *addr);
