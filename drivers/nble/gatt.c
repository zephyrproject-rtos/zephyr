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
#include <bluetooth/log.h>

#include "gatt_internal.h"

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

int bt_gatt_notify(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		   const void *data, uint16_t len)
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

void on_ble_gattc_write_rsp(const struct ble_gattc_write_rsp *ev,
			    void *priv)
{
	BT_DBG("");
}

void on_ble_gattc_read_rsp(const struct ble_gattc_read_rsp *ev,
			   uint8_t *data, uint8_t data_len, void *priv)
{
	BT_DBG("");
}

void on_ble_gattc_value_evt(const struct ble_gattc_value_evt *ev,
		uint8_t *buf, uint8_t buflen)
{
	BT_DBG("");
}

void on_ble_gatts_write_evt(const struct ble_gatt_wr_evt *ev,
			    const uint8_t *buf, uint8_t buflen)
{
	BT_DBG("");
}

void on_ble_gatts_get_attribute_value_rsp(const struct ble_gatts_attribute_response *par,
					  uint8_t *data, uint8_t length)
{
	BT_DBG("");
}

void on_ble_gatt_register_rsp(const struct ble_gatt_register_rsp *rsp,
			      const struct ble_gatt_attr_handles *handles,
			      uint8_t len)
{
	BT_DBG("");
}

void on_ble_gattc_discover_rsp(const struct ble_gattc_disc_rsp *rsp,
			       const uint8_t *data, uint8_t len)
{
	BT_DBG("");
}

void on_ble_gatts_send_svc_changed_rsp(const struct ble_core_response *par)
{
	BT_DBG("");
}

void on_ble_gatts_set_attribute_value_rsp(const struct ble_gatts_attribute_response *par)
{
	BT_DBG("");
}

void on_ble_gatts_send_notif_ind_rsp(const struct ble_gatt_notif_ind_rsp *par)
{
	BT_DBG("");
}
