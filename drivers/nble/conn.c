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
#include <bluetooth/conn.h>

struct bt_conn *bt_conn_ref(struct bt_conn *conn)
{
	return NULL;
}

void bt_conn_unref(struct bt_conn *conn)
{
}

struct bt_conn *bt_conn_lookup_addr_le(const bt_addr_le_t *peer)
{
	return NULL;
}

const bt_addr_le_t *bt_conn_get_dst(const struct bt_conn *conn)
{
	return NULL;
}

int bt_conn_get_info(const struct bt_conn *conn, struct bt_conn_info *info)
{
	return -ENOSYS;
}

int bt_conn_le_param_update(struct bt_conn *conn,
			    const struct bt_le_conn_param *param)
{
	return -ENOSYS;
}

int bt_conn_disconnect(struct bt_conn *conn, uint8_t reason)
{
	return -ENOSYS;
}

struct bt_conn *bt_conn_create_le(const bt_addr_le_t *peer,
				  const struct bt_le_conn_param *param)
{
	return NULL;
}

int bt_conn_security(struct bt_conn *conn, bt_security_t sec)
{
	return -ENOSYS;
}

uint8_t bt_conn_enc_key_size(struct bt_conn *conn)
{
	return 0;
}

void bt_conn_cb_register(struct bt_conn_cb *cb)
{
}

int bt_le_set_auto_conn(bt_addr_le_t *addr,
			const struct bt_le_conn_param *param)
{
	return -ENOSYS;
}

struct bt_conn *bt_conn_create_slave_le(const bt_addr_le_t *peer,
					const struct bt_le_adv_param *param)
{
	return NULL;
}

int bt_conn_auth_cb_register(const struct bt_conn_auth_cb *cb)
{
	return -ENOSYS;
}

int bt_conn_auth_passkey_entry(struct bt_conn *conn, unsigned int passkey)
{
	return -ENOSYS;
}

int bt_conn_auth_cancel(struct bt_conn *conn)
{
	return -ENOSYS;
}

int bt_conn_auth_passkey_confirm(struct bt_conn *conn, bool match)
{
	return -ENOSYS;
}
