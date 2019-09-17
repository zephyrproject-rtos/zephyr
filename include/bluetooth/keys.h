/** @file
 *  @brief Bluetooth key handling.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_KEYS_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_KEYS_H_

#include <bluetooth/hci.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
	BT_KEYS_SLAVE_LTK      = BIT(0),
	BT_KEYS_IRK            = BIT(1),
	BT_KEYS_LTK            = BIT(2),
	BT_KEYS_LOCAL_CSRK     = BIT(3),
	BT_KEYS_REMOTE_CSRK    = BIT(4),
	BT_KEYS_LTK_P256       = BIT(5),

	BT_KEYS_ALL            = (BT_KEYS_SLAVE_LTK | BT_KEYS_IRK |  \
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
	u8_t rand[8];
	u8_t ediv[2];
	u8_t val[16];
};

struct bt_irk {
	u8_t val[16];
	bt_addr_t rpa;
};

struct bt_csrk {
	u8_t val[16];
	u32_t cnt;
};

struct bt_keys {
	u8_t id;
	bt_addr_le_t addr;
	u8_t storage_start[0];
	u8_t enc_size;
	u8_t flags;
	u16_t keys;
	struct bt_ltk ltk;
	struct bt_irk irk;
#if defined(CONFIG_BT_SIGNING)
	struct bt_csrk local_csrk;
	struct bt_csrk remote_csrk;
#endif  /* BT_SIGNING */
#if !defined(CONFIG_BT_SMP_SC_PAIR_ONLY)
	struct bt_ltk slave_ltk;
#endif  /* CONFIG_BT_SMP_SC_PAIR_ONLY */
};

#define BT_KEYS_STORAGE_LEN     (sizeof(struct bt_keys) - \
				 offsetof(struct bt_keys, storage_start))

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_KEYS_H_ */
