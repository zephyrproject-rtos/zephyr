/* This is a stub file for the linker. None of the sections are used by the test. */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

SECTIONS
{
	bt_conn_cb_area : ALIGN(4)
	{
		_bt_conn_cb_list_start = .;
		KEEP(*(SORT_BY_NAME(._bt_conn_cb.static.*)))
		_bt_conn_cb_list_end = .;
	}
}
INSERT AFTER .data;

SECTIONS
{
	net_buf_pool : ALIGN(4)
	{
		_net_buf_pool_list_start = .;
		KEEP(*(SORT_BY_NAME(._net_buf_pool.static.*)))
		_net_buf_pool_list_end_end = .;
	}
}
INSERT AFTER .bss;
