/* Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SETTINGS_ZMS_H_
#define __SETTINGS_ZMS_H_

#include <zephyr/fs/zms.h>
#include <zephyr/settings/settings.h>

#ifdef __cplusplus
extern "C" {
#endif

/* In the ZMS backend, each setting is stored in two ZMS entries:
 *	1. setting's name
 *	2. setting's value
 *
 * The ZMS entry ID for the setting's value is determined implicitly based on
 * the ID of the ZMS entry for the setting's name, once that is found. The
 * difference between name and value ID is constant and equal to
 * ZMS_NAME_ID_OFFSET.
 *
 * Setting's name is hashed into 29 bits minus hash_collisions_bits.
 * The 2 MSB_bits have always the same value 10, the LL_bit for the name's hash is 0
 * and the hash_collisions_bits is configurable through CONFIG_SETTINGS_ZMS_MAX_COLLISIONS_BITS.
 * The resulted 32 bits is the ZMS_ID of the Setting's name.
 * If we detect a collision between ZMS_IDs we increment the value within hash_collision_bits
 * until we find a free ZMS_ID.
 * Separately, we store a linked list using the Setting's name ZMS_ID but setting the lsb to 1.
 *
 * The linked list is used to maintain a relation between all ZMS_IDs. This is necessary to load
 * all settings at initialization.
 * The linked list contains at least a header followed by multiple linked list elements that
 * we can refer to as LL_x (where x is the order of that element in that list).
 * This is a representation of the Linked List that is stored in the storage.
 * LL_header <--> LL_0 <--> LL_1 <--> LL_2.
 * The "next_hash" pointer of each LL element refers to the next element in the linked list.
 * The "previous_hash" pointer is referring the previous element in the linked list.
 *
 * The bit representation of the 32 bits ZMS_ID is the following:
 * --------------------------------------------------------------
 * | MSB_bits | hash (truncated) | hash_collision_bits | LL_bit |
 * --------------------------------------------------------------
 * Where:
 * MSB_bits (2 bits width) : = 10 for Name IDs
 *			     = 11 for Data IDs
 * hash (29 bits - hash_collision_bits) : truncated hash obtained from sys_hash32
 * hash_collision_bits (configurable width) : used to handle hash collisions
 * LL_bit : = 0 when this is a name's ZMS_ID
 *	    = 1 when this is the linked list ZMS_ID corresponding to the name
 *
 * if a settings element is deleted it won't be found.
 */

#define ZMS_LL_HEAD_HASH_ID 0x80000000
#define ZMS_DATA_ID_OFFSET  0x40000000
#define ZMS_HASH_MASK       GENMASK(29, CONFIG_SETTINGS_ZMS_MAX_COLLISIONS_BITS + 1)
#define ZMS_COLLISIONS_MASK GENMASK(CONFIG_SETTINGS_ZMS_MAX_COLLISIONS_BITS, 1)
#define ZMS_HASH_TOTAL_MASK GENMASK(29, 1)
#define ZMS_MAX_COLLISIONS  (BIT(CONFIG_SETTINGS_ZMS_MAX_COLLISIONS_BITS) - 1)

/* some useful macros */
#define ZMS_NAME_ID_FROM_LL_NODE(x) (x & ~BIT(0))
#define ZMS_UPDATE_COLLISION_NUM(x, y)                                                             \
	((x & ~ZMS_COLLISIONS_MASK) | ((y << 1) & ZMS_COLLISIONS_MASK))
#define ZMS_COLLISION_NUM(x) ((x & ZMS_COLLISIONS_MASK) >> 1)

struct settings_zms {
	struct settings_store cf_store;
	struct zms_fs cf_zms;
	const struct device *flash_dev;
	uint32_t last_hash_id;
	uint32_t second_to_last_hash_id;
	uint8_t hash_collision_num;
};

struct settings_hash_linked_list {
	uint32_t previous_hash;
	uint32_t next_hash;
};

#ifdef __cplusplus
}
#endif

#endif /* __SETTINGS_ZMS_H_ */
