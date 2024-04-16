/*
 * Copyright 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

SECTIONS
{
	data.bt_conn_cb_area : ALIGN(4)
	{
		_bt_conn_cb_list_start = .;
		KEEP(*(SORT_BY_NAME(._bt_conn_cb.static.*)))
		_bt_conn_cb_list_end = .;
	}
	data.bt_gatt_service_static_area : ALIGN(4)
	{
		_bt_gatt_service_static_list_start = .;
		KEEP(*(SORT_BY_NAME(._bt_gatt_service_static.static.*)))
		_bt_gatt_service_static_list_end = .;
	}
}
INSERT AFTER .data;
