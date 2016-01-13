/*
 * Copyright (c) 2016 Intel Corporation
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

#include <errno.h>

#include <bluetooth/bluetooth.h>

int bt_enable(bt_ready_cb_t cb)
{
	return -ENOSYS;
}

int bt_le_adv_start(const struct bt_le_adv_param *param,
		    const struct bt_data *ad, size_t ad_len,
		    const struct bt_data *sd, size_t sd_len)
{
	return -ENOSYS;
}

int bt_le_adv_stop(void)
{
	return -ENOSYS;
}

int bt_le_scan_start(const struct bt_le_scan_param *param, bt_le_scan_cb_t cb)
{
	return -ENOSYS;
}

int bt_le_scan_stop(void)
{
	return -ENOSYS;
}

int bt_le_set_auto_conn(bt_addr_le_t *addr,
			const struct bt_le_conn_param *param)
{
	return -ENOSYS;
}

int bt_auth_cb_register(const struct bt_auth_cb *cb)
{
	return -ENOSYS;
}

void bt_auth_passkey_entry(struct bt_conn *conn, unsigned int passkey)
{
}

void bt_auth_cancel(struct bt_conn *conn)
{
}

void bt_auth_passkey_confirm(struct bt_conn *conn, bool match)
{
}
