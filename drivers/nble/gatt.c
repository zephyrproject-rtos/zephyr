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

#include <bluetooth/gatt.h>

int bt_gatt_register(struct bt_gatt_attr *attrs, size_t count)
{
	return -ENOSYS;
}

void bt_gatt_foreach_attr(uint16_t start_handle, uint16_t end_handle,
			  bt_gatt_attr_func_t func, void *user_data)
{
}

struct bt_gatt_attr *bt_gatt_attr_next(const struct bt_gatt_attr *attr)
{
	return NULL;
}

int bt_gatt_attr_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		      void *buf, uint16_t buf_len, uint16_t offset,
		      const void *value, uint16_t value_len)
{
	return -ENOSYS;
}

int bt_gatt_attr_read_service(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr,
			      void *buf, uint16_t len, uint16_t offset)
{
	return -ENOSYS;
}

int bt_gatt_attr_read_included(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr,
			       void *buf, uint16_t len, uint16_t offset)
{
	return -ENOSYS;
}

int bt_gatt_attr_read_chrc(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	return -ENOSYS;
}

int bt_gatt_attr_read_ccc(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, void *buf,
			  uint16_t len, uint16_t offset)
{
	return -ENOSYS;
}

int bt_gatt_attr_write_ccc(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, const void *buf,
			   uint16_t len, uint16_t offset)
{
	return -ENOSYS;
}

int bt_gatt_attr_read_cep(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, void *buf,
			  uint16_t len, uint16_t offset)
{
	return -ENOSYS;
}

int bt_gatt_attr_read_cud(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, void *buf,
			  uint16_t len, uint16_t offset)
{
	return -ENOSYS;
}

int bt_gatt_attr_read_cpf(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, void *buf,
			  uint16_t len, uint16_t offset)
{
	return -ENOSYS;
}

int bt_gatt_notify(struct bt_conn *conn, uint16_t handle, const void *data,
		   uint16_t len)
{
	return -ENOSYS;
}

int bt_gatt_exchange_mtu(struct bt_conn *conn, bt_gatt_rsp_func_t func)
{
	return -ENOSYS;
}

int bt_gatt_discover(struct bt_conn *conn,
		     struct bt_gatt_discover_params *params)
{
	return -ENOSYS;
}

int bt_gatt_read(struct bt_conn *conn, struct bt_gatt_read_params *params)
{
	return -ENOSYS;
}

int bt_gatt_write(struct bt_conn *conn, uint16_t handle, uint16_t offset,
		  const void *data, uint16_t length, bt_gatt_rsp_func_t func)
{
	return -ENOSYS;
}

int bt_gatt_write_without_response(struct bt_conn *conn, uint16_t handle,
				   const void *data, uint16_t length,
				   bool sign)
{
	return -ENOSYS;
}

int bt_gatt_subscribe(struct bt_conn *conn,
		      struct bt_gatt_subscribe_params *params)
{
	return -ENOSYS;
}

int bt_gatt_unsubscribe(struct bt_conn *conn,
			struct bt_gatt_subscribe_params *params)
{
	return -ENOSYS;
}

void bt_gatt_cancel(struct bt_conn *conn)
{
}

int bt_gatt_read_multiple(struct bt_conn *conn, const uint16_t *handles,
			  size_t count, bt_gatt_read_func_t func)
{
	return -ENOSYS;
}
