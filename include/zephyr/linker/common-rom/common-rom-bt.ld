/* SPDX-License-Identifier: Apache-2.0 */

#include <zephyr/linker/iterable_sections.h>

	ITERABLE_SECTION_ROM(bt_l2cap_fixed_chan, Z_LINK_ITERABLE_SUBALIGN)

#if defined(CONFIG_BT_CLASSIC)
	ITERABLE_SECTION_ROM(bt_l2cap_br_fixed_chan, Z_LINK_ITERABLE_SUBALIGN)
#endif

#if defined(CONFIG_BT_CONN)
	ITERABLE_SECTION_ROM(bt_conn_cb, Z_LINK_ITERABLE_SUBALIGN)
#endif

	ITERABLE_SECTION_ROM(bt_gatt_service_static, Z_LINK_ITERABLE_SUBALIGN)

#if defined(CONFIG_BT_MESH)
	ITERABLE_SECTION_ROM(bt_mesh_subnet_cb, Z_LINK_ITERABLE_SUBALIGN)
	ITERABLE_SECTION_ROM(bt_mesh_app_key_cb, Z_LINK_ITERABLE_SUBALIGN)

	ITERABLE_SECTION_ROM(bt_mesh_hb_cb, Z_LINK_ITERABLE_SUBALIGN)

#if defined(CONFIG_BT_TESTING)
	ITERABLE_SECTION_ROM(bt_mesh_beacon_cb, Z_LINK_ITERABLE_SUBALIGN)
#endif
#endif

#if defined(CONFIG_BT_MESH_FRIEND)
	ITERABLE_SECTION_ROM(bt_mesh_friend_cb, Z_LINK_ITERABLE_SUBALIGN)
#endif

#if defined(CONFIG_BT_MESH_LOW_POWER)
	ITERABLE_SECTION_ROM(bt_mesh_lpn_cb, Z_LINK_ITERABLE_SUBALIGN)
#endif

#if defined(CONFIG_BT_IAS)
	ITERABLE_SECTION_ROM(bt_ias_cb, Z_LINK_ITERABLE_SUBALIGN)
#endif

#if defined(CONFIG_BT_MESH_GATT_PROXY)
	ITERABLE_SECTION_ROM(bt_mesh_proxy_cb, Z_LINK_ITERABLE_SUBALIGN)
#endif
