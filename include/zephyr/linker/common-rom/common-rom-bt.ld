/* SPDX-License-Identifier: Apache-2.0 */

	ITERABLE_SECTION_ROM(bt_l2cap_fixed_chan, 4)

#if defined(CONFIG_BT_BREDR)
	ITERABLE_SECTION_ROM(bt_l2cap_br_fixed_chan, 4)
#endif

#if defined(CONFIG_BT_CONN)
	ITERABLE_SECTION_ROM(bt_conn_cb, 4)
#endif

	ITERABLE_SECTION_ROM(bt_gatt_service_static, 4)

#if defined(CONFIG_BT_MESH)
	ITERABLE_SECTION_ROM(bt_mesh_subnet_cb, 4)
	ITERABLE_SECTION_ROM(bt_mesh_app_key_cb, 4)

	ITERABLE_SECTION_ROM(bt_mesh_hb_cb, 4)
#endif

#if defined(CONFIG_BT_MESH_FRIEND)
	ITERABLE_SECTION_ROM(bt_mesh_friend_cb, 4)
#endif

#if defined(CONFIG_BT_MESH_LOW_POWER)
	ITERABLE_SECTION_ROM(bt_mesh_lpn_cb, 4)
#endif

#if defined(CONFIG_BT_IAS)
	ITERABLE_SECTION_ROM(bt_ias_cb, 4)
#endif

#if defined(CONFIG_BT_MESH_GATT_PROXY)
	ITERABLE_SECTION_ROM(bt_mesh_proxy_cb, 4)
#endif
