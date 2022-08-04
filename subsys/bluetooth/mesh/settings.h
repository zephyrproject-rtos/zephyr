/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Pending storage actions. */
enum bt_mesh_settings_flag {
	BT_MESH_SETTINGS_RPL_PENDING,
	BT_MESH_SETTINGS_NET_KEYS_PENDING,
	BT_MESH_SETTINGS_APP_KEYS_PENDING,
	BT_MESH_SETTINGS_NET_PENDING,
	BT_MESH_SETTINGS_IV_PENDING,
	BT_MESH_SETTINGS_SEQ_PENDING,
	BT_MESH_SETTINGS_HB_PUB_PENDING,
	BT_MESH_SETTINGS_CFG_PENDING,
	BT_MESH_SETTINGS_MOD_PENDING,
	BT_MESH_SETTINGS_VA_PENDING,
	BT_MESH_SETTINGS_CDB_PENDING,

	BT_MESH_SETTINGS_FLAG_COUNT,
};

#ifdef CONFIG_BT_SETTINGS
#define BT_MESH_SETTINGS_DEFINE(_hname, _subtree, _set)			     \
	SETTINGS_STATIC_HANDLER_DEFINE(bt_mesh_##_hname, "bt/mesh/" _subtree, \
				       NULL, _set, NULL, NULL)
#else
/* Declaring non static settings handler helps avoid unnecessary ifdefs
 * as well as unused function warning. Since the declared handler structure is
 * unused, linker will discard it.
 */
#define BT_MESH_SETTINGS_DEFINE(_hname, _subtree, _set)\
	const struct settings_handler settings_handler_bt_mesh_ ## _hname = {\
		.h_set = _set,						     \
	}
#endif

void bt_mesh_settings_init(void);
void bt_mesh_settings_store_schedule(enum bt_mesh_settings_flag flag);
void bt_mesh_settings_store_cancel(enum bt_mesh_settings_flag flag);
void bt_mesh_settings_store_pending(void);
int bt_mesh_settings_set(settings_read_cb read_cb, void *cb_arg,
			 void *out, size_t read_len);
