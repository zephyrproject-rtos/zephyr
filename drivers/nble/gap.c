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

#include "uart.h"
#include "rpc.h"

#define NBLE_SWDIO_PIN	6
#define NBLE_RESET_PIN	NBLE_SWDIO_PIN

#define NBLE_CHANNEL	0

static void *channel;

/**
 * Function handling the events from the UART IPC (irq).
 *
 * @param channel Channel on which the event applies
 * @param request IPC_MSG_TYPE_MESSAGE when a new message was received,
 * IPC_MSG_TYPE_FREE when the previous message was sent and can be freed.
 * @param len Length of the data
 * @param p_data Pointer to the data
 * @return 0
 */
static int recv_cb(int channel, int request, int len, void *p_data)
{
	BT_DBG("channel %d request %d len %d", channel, request, len);

	switch (request) {
	case IPC_MSG_TYPE_MESSAGE:
		break;
	case IPC_MSG_TYPE_FREE:
		/* TODO: Try to send another message immediately */
		break;
	default:
		/* Free the message */
		BT_ERR("Unsupported RPC request");
		break;
	}

	return 0;
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

	/* Open the UART channel for RPC while Nordic is in reset */
	channel = ipc_uart_channel_open(NBLE_CHANNEL, recv_cb);
	BT_DBG("channel %p", channel);

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
