/* keys.h - Bluetooth key handling */

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

#if defined(CONFIG_BLUETOOTH_SMP)
enum {
	BT_KEYS_SLAVE_LTK      = (1 << 0),
	BT_KEYS_IRK            = (1 << 1),
	BT_KEYS_LTK            = (1 << 2),
	BT_KEYS_LOCAL_CSRK     = (1 << 3),
	BT_KEYS_REMOTE_CSRK    = (1 << 4),

	BT_KEYS_ALL            = (BT_KEYS_SLAVE_LTK | BT_KEYS_IRK | \
				  BT_KEYS_LTK | BT_KEYS_LOCAL_CSRK | \
				  BT_KEYS_REMOTE_CSRK),
};

enum {
	BT_KEYS_UNAUTHENTICATED,
	BT_KEYS_AUTHENTICATED,
};

struct bt_ltk {
	uint64_t		rand;
	uint16_t		ediv;
	uint8_t			val[16];
};

struct bt_irk {
	uint8_t			val[16];
	bt_addr_t		rpa;
};

struct bt_csrk {
	uint8_t			val[16];
	uint32_t		cnt;
};

struct bt_keys {
	bt_addr_le_t		addr;
	int			keys;
	uint8_t			type;

	struct bt_ltk		slave_ltk;
	struct bt_ltk		ltk;
	struct bt_irk		irk;
#if defined(CONFIG_BLUETOOTH_SIGNING)
	struct bt_csrk		local_csrk;
	struct bt_csrk		remote_csrk;
#endif /* BLUETOOTH_SIGNING */
};

struct bt_keys *bt_keys_get_addr(const bt_addr_le_t *addr);
struct bt_keys *bt_keys_get_type(int type, const bt_addr_le_t *addr);
void bt_keys_add_type(struct bt_keys *keys, int type);
void bt_keys_clear(struct bt_keys *keys, int type);
struct bt_keys *bt_keys_find(int type, const bt_addr_le_t *addr);
struct bt_keys *bt_keys_find_irk(const bt_addr_le_t *addr);
struct bt_keys *bt_keys_find_addr(const bt_addr_le_t *addr);
#else
static inline struct bt_keys *bt_keys_find_addr(const bt_addr_le_t *addr)
{
	return NULL;
}
#endif /* CONFIG_BLUETOOTH_SMP */
