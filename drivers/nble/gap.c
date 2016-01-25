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

#include <nanokernel.h>
#include <device.h>
#include <gpio.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/log.h>

#include "gap_internal.h"
#include "uart.h"
#include "rpc.h"

#define NBLE_SWDIO_PIN	6
#define NBLE_RESET_PIN	NBLE_SWDIO_PIN
#define NBLE_BTWAKE_PIN 5

static bt_ready_cb_t bt_ready_cb;

void on_nble_up(void)
{
	BT_DBG("");
	if (bt_ready_cb) {
		bt_ready_cb(0);
	}

	ble_get_version_req(NULL);
}

void on_ble_get_version_rsp(const struct ble_version_response *rsp)
{
	BT_DBG("");
}

int bt_enable(bt_ready_cb_t cb)
{
	struct device *gpio;
	int ret;

	BT_DBG("");

	gpio = device_get_binding(CONFIG_GPIO_DW_0_NAME);
	if (!gpio) {
		BT_ERR("Cannot find %", CONFIG_GPIO_DW_0_NAME);
		return -ENODEV;
	}

	ret = gpio_pin_configure(gpio, NBLE_RESET_PIN, GPIO_DIR_OUT);
	if (ret) {
		BT_ERR("Error configuring pin %d", NBLE_RESET_PIN);
		return -ENODEV;
	}

	/* Reset hold time is 0.2us (normal) or 100us (SWD debug) */
	ret = gpio_pin_write(gpio, NBLE_RESET_PIN, 0);
	if (ret) {
		BT_ERR("Error pin write %d", NBLE_RESET_PIN);
		return -EINVAL;
	}

	ret = gpio_pin_configure(gpio, NBLE_BTWAKE_PIN, GPIO_DIR_OUT);
	if (ret) {
		BT_ERR("Error configuring pin %d", NBLE_BTWAKE_PIN);
		return -ENODEV;
	}

	ret = gpio_pin_write(gpio, NBLE_BTWAKE_PIN, 1);
	if (ret) {
		BT_ERR("Error pin write %d", NBLE_BTWAKE_PIN);
		return -EINVAL;
	}

	/**
	 * NBLE reset is achieved by asserting low the SWDIO pin.
	 * However, the BLE Core chip can be in SWD debug mode,
	 * and NRF_POWER->RESET = 0 due to, other constraints: therefore,
	 * this reset might not work everytime, especially after
	 * flashing or debugging.
	 */

	/* sleep 1ms depending on context */
	switch (sys_execution_context_type_get()) {
	case NANO_CTX_FIBER:
		fiber_sleep(MSEC(1));
		break;
	case NANO_CTX_TASK:
		task_sleep(MSEC(1));
		break;
	default:
		BT_ERR("ISR context is not supported");
		return -EINVAL;
	}

	ret = nble_open();
	if (ret) {
		return ret;
	}

	ret = gpio_pin_write(gpio, NBLE_RESET_PIN, 1);
	if (ret) {
		BT_ERR("Error pin write %d", NBLE_RESET_PIN);
		return -EINVAL;
	}

	/* Set back GPIO to input to avoid interfering with external debugger */
	ret = gpio_pin_configure(gpio, NBLE_RESET_PIN, GPIO_DIR_IN);
	if (ret) {
		BT_ERR("Error configuring pin %d", NBLE_RESET_PIN);
		return -ENODEV;
	}

	bt_ready_cb = cb;

	return 0;
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

void on_ble_gap_dtm_init_rsp(void *user_data)
{
	BT_DBG("");
}

void ble_log(const struct ble_log_s *param, char *format, uint8_t len)
{
	BT_DBG("");
}

void on_ble_gap_connect_evt(const struct ble_gap_connect_evt *ev)
{
	BT_DBG("");
}


void on_ble_gap_disconnect_evt(const struct ble_gap_disconnect_evt *ev)
{
	BT_DBG("");
}

void on_ble_gap_conn_update_evt(const struct ble_gap_conn_update_evt *ev)
{
	BT_DBG("");
}

void on_ble_gap_sm_status_evt(const struct ble_gap_sm_status_evt *ev)
{
	BT_DBG("");
}

void on_ble_gap_sm_passkey_display_evt(const struct ble_gap_sm_passkey_disp_evt *ev)
{
	BT_DBG("");
}

void on_ble_gap_sm_passkey_req_evt(const struct ble_gap_sm_passkey_req_evt *ev)
{
	BT_DBG("");
}

void on_ble_gap_to_evt(const struct ble_gap_timout_evt *ev)
{
	BT_DBG("");
}

void on_ble_gap_rssi_evt(const struct ble_gap_rssi_evt *ev)
{
	BT_DBG("");
}

void on_ble_gap_service_read_rsp(const struct ble_core_response *par)
{
	BT_DBG("");
}

void on_ble_gap_read_bda_rsp(const struct ble_service_read_bda_response *par)
{
	BT_DBG("");
}

void on_ble_gap_disconnect_rsp(const struct ble_core_response *par)
{
	BT_DBG("");
}

void on_ble_gap_sm_pairing_rsp(const struct ble_core_response *par)
{
	BT_DBG("");
}

void on_ble_gap_sm_config_rsp(struct ble_gap_sm_config_rsp *par)
{
	BT_DBG("");
}

void on_ble_gap_clr_white_list_rsp(const struct ble_core_response *par)
{
	BT_DBG("");
}

void on_ble_gap_sm_passkey_reply_rsp(const struct ble_core_response *par)
{
	BT_DBG("");
}

void on_ble_gap_connect_rsp(const struct ble_core_response *par)
{
	BT_DBG("");
}

void on_ble_gap_start_scan_rsp(const struct ble_core_response *par)
{
	BT_DBG("");
}

void on_ble_gap_stop_scan_rsp(const struct ble_core_response *par)
{
	BT_DBG("");
}

void on_ble_gap_cancel_connect_rsp(const struct ble_core_response *par)
{
	BT_DBG("");
}

void on_ble_gap_set_option_rsp(const struct ble_core_response *par)
{
	BT_DBG("");
}

void on_ble_gap_generic_cmd_rsp(const struct ble_core_response *par)
{
	BT_DBG("");
}

void on_ble_gap_conn_update_rsp(const struct ble_core_response *par)
{
	BT_DBG("");
}

void on_ble_gap_sm_clear_bonds_rsp(const struct ble_core_response *par)
{
	BT_DBG("");
}

void on_ble_gap_service_write_rsp(const struct ble_service_write_response *par)
{
	BT_DBG("");
}

void on_ble_set_enable_config_rsp(const struct ble_core_response *par)
{
	BT_DBG("");
}

void on_ble_gap_set_rssi_report_rsp(const struct ble_core_response *par)
{
	BT_DBG("");
}

void on_ble_gap_wr_white_list_rsp(const struct ble_core_response *par)
{
	BT_DBG("");
}

void on_ble_gap_dbg_rsp(const struct debug_response *par)
{
	BT_DBG("");
}

void on_ble_get_bonded_device_list_rsp(const struct ble_get_bonded_device_list_rsp *par)
{
	BT_DBG("");
}

void on_ble_gap_start_advertise_rsp(const struct ble_core_response *par)
{
	BT_DBG("");
}


void on_ble_gap_stop_advertise_rsp(const struct ble_core_response *par)
{
	BT_DBG("");
}
