/* keys.h - Bluetooth key handling */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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
	bt_addr_t addr;
	u8_t flags;
	u8_t val[16];
};

struct bt_keys_link_key *bt_keys_get_link_key(const bt_addr_t *addr);
struct bt_keys_link_key *bt_keys_find_link_key(const bt_addr_t *addr);
void bt_keys_link_key_clear(struct bt_keys_link_key *link_key);
void bt_keys_link_key_clear_addr(const bt_addr_t *addr);
