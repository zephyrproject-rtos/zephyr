/* SiLabs WFX200 WiFi Module with SPI
 * or SDIO Interface
 *
 * Copyright (c) 2022 grandcentrix GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#define LOG_MODULE_NAME wifi_wfx200
#define LOG_LEVEL CONFIG_WIFI_LOG_LEVEL

#include <sl_wfx.h>
#undef BIT

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <string.h>
#include <errno.h>

#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>
#include <drivers/spi.h>
#include <net/net_pkt.h>
#include <net/net_if.h>
#include <net/ethernet.h>

#include "wfx200_internal.h"

static void wfx200_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct wfx200_data *context = dev->data;

	LOG_DBG("Interface init");
	context->iface = iface;
	net_if_set_link_addr(iface, context->sl_context.mac_addr_0.octet,
			     sizeof(context->sl_context.mac_addr_0.octet),
			     NET_LINK_ETHERNET);
	ethernet_init(iface);
	context->state = WFX200_STATE_INTERFACE_INITIALIZED;
	net_if_flag_set(iface, NET_IF_NO_AUTO_START);
}

int wfx200_send(const struct device *dev, struct net_pkt *pkt)
{
	struct wfx200_data *context = dev->data;
	size_t len = net_pkt_get_len(pkt), frame_len;
	sl_wfx_send_frame_req_t *tx_buffer;
	sl_status_t result;
	sl_wfx_interface_t interface;
	int status = 0;

	switch (context->state) {
	case WFX200_STATE_AP_MODE:
		if (!(context->sl_context.state & SL_WFX_AP_INTERFACE_UP)) {
			status = -EIO;
			goto out;
		}
		interface = SL_WFX_SOFTAP_INTERFACE;
		break;
	case WFX200_STATE_STA_MODE:
		if (!(context->sl_context.state & SL_WFX_STA_INTERFACE_CONNECTED)) {
			status = -EIO;
			goto out;
		}
		interface = SL_WFX_STA_INTERFACE;
		break;
	default:
		status = -ENODEV;
		goto out;
	}
	frame_len = SL_WFX_ROUND_UP(len, 2);

	result = sl_wfx_allocate_command_buffer((sl_wfx_generic_message_t **)&tx_buffer,
						SL_WFX_SEND_FRAME_REQ_ID,
						SL_WFX_TX_FRAME_BUFFER,
						frame_len + sizeof(sl_wfx_send_frame_req_t));
	if (result != SL_STATUS_OK) {
		status = -ENOMEM;
		goto out;
	}

	if (net_pkt_read(pkt, tx_buffer->body.packet_data, len)) {
		status = -EIO;
		goto out_free;
	}

	result = sl_wfx_send_ethernet_frame(tx_buffer,
					    frame_len,
					    interface,
					    WFM_PRIORITY_BE0);
	if (result != SL_STATUS_OK) {
		status = -EIO;
		goto out_free;
	}

out_free:
	sl_wfx_free_command_buffer((sl_wfx_generic_message_t *)tx_buffer,
				   SL_WFX_SEND_FRAME_REQ_ID,
				   SL_WFX_TX_FRAME_BUFFER);
out:
	return status;
}

static enum ethernet_hw_caps wfx200_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

int wfx200_scan(const struct device *dev, scan_result_cb_t cb)
{
	struct wfx200_data *context = dev->data;
	sl_status_t result;

	if (context->state < WFX200_STATE_INTERFACE_INITIALIZED) {
		return -EIO;
	}
	if (context->scan_cb != NULL) {
		return -EINPROGRESS;
	}
	context->scan_cb = cb;
	result = sl_wfx_send_scan_command(WFM_SCAN_MODE_PASSIVE,
					  NULL,
					  0,
					  NULL,
					  0,
					  NULL,
					  0,
					  NULL);
	if (result != SL_STATUS_OK) {
		context->scan_cb = NULL;
		return -EIO;
	}
	return 0;
}

int wfx200_connect(const struct device *dev, struct wifi_connect_req_params *params)
{
	struct wfx200_data *context = dev->data;
	char ssid[WIFI_SSID_MAX_LEN + 1];
	sl_wfx_security_mode_t mode;
	sl_status_t result;
	uint16_t channel;

	switch (context->state) {
	case WFX200_STATE_INTERFACE_INITIALIZED:
		break;
	case WFX200_STATE_AP_MODE:
		return -EBUSY;
	case WFX200_STATE_STA_MODE:
		return -EALREADY;
	default:
		return -ENODEV;
	}
	memcpy(ssid, params->ssid, MIN(params->ssid_length, sizeof(ssid)));
	ssid[MIN(params->ssid_length, WIFI_SSID_MAX_LEN)] = 0;

	if (params->security == WIFI_SECURITY_TYPE_PSK) {
		mode = WFM_SECURITY_MODE_WPA2_WPA1_PSK;
	} else if (params->security == WIFI_SECURITY_TYPE_NONE) {
		mode = WFM_SECURITY_MODE_OPEN;
	} else {
		LOG_ERR("Not supported security mode");
		return -ENOTSUP;
	}

	if (params->channel <= WIFI_CHANNEL_MAX) {
		channel = params->channel;
	} else {
		channel = 0;
	}

	LOG_INF("Connecting to %s on channel %d", log_strdup(ssid), channel);

	result = sl_wfx_send_join_command(params->ssid,
					  params->ssid_length,
					  NULL,
					  channel,
					  mode,
					  0,
					  0,
					  (const uint8_t *)params->psk,
					  params->psk_length,
					  NULL,
					  0
					  );
	if (result != SL_STATUS_OK) {
		LOG_DBG("Failed to send join command: %d", result);
		return -EIO;
	}
	context->state = WFX200_STATE_STA_MODE;

	return 0;
}

int wfx200_disconnect(const struct device *dev)
{
	struct wfx200_data *context = dev->data;
	sl_status_t result;

	if (context->state < WFX200_STATE_INTERFACE_INITIALIZED) {
		return -ENODEV;
	}
	if (context->sl_context.state & SL_WFX_STA_INTERFACE_CONNECTED) {
		result = sl_wfx_send_disconnect_command();
		if (result != SL_STATUS_OK) {
			return -EIO;
		}
	}
	return 0;
}

int wfx200_ap_enable(const struct device *dev, struct wifi_connect_req_params *params)
{
	struct wfx200_data *context = dev->data;
	sl_wfx_status_t result;

	switch (context->state) {
	case WFX200_STATE_INTERFACE_INITIALIZED:
		break;
	case WFX200_STATE_STA_MODE:
		return -EBUSY;
	case WFX200_STATE_AP_MODE:
		return -EALREADY;
	default:
		return -ENODEV;
	}
	if (params->channel < 1 || params->channel > 14) {
		return -ERANGE;
	}
	if (params->ssid_length < 1 || params->ssid_length > WIFI_SSID_MAX_LEN) {
		return -EINVAL;
	}
	if (params->ssid == NULL) {
		return -EINVAL;
	}
	if (params->security == WIFI_SECURITY_TYPE_PSK) {
		if (params->psk_length < 8 || params->psk_length > WIFI_PSK_MAX_LEN) {
			return -EINVAL;
		}
		if (params->psk == NULL) {
			return -EINVAL;
		}
	}

	result = sl_wfx_start_ap_command(
		params->channel,
		params->ssid,
		params->ssid_length,
		0,
		0,
		(params->security == WIFI_SECURITY_TYPE_PSK)?
		WFM_SECURITY_MODE_WPA2_PSK:
		WFM_SECURITY_MODE_OPEN,
		0,
		params->psk,
		params->psk_length,
		NULL,
		0,
		NULL,
		0
		);
	if (result != SL_STATUS_OK) {
		LOG_DBG("Start AP command failed: %d", result);
		return -EIO;
	}
	context->state = WFX200_STATE_AP_MODE;

	return 0;
}

int wfx200_ap_disable(const struct device *dev)
{
	struct wfx200_data *context = dev->data;
	sl_status_t result;

	if (context->state < WFX200_STATE_INTERFACE_INITIALIZED) {
		return -ENODEV;
	}
	if (context->sl_context.state & SL_WFX_AP_INTERFACE_UP) {
		result = sl_wfx_stop_ap_command();
		if (result != SL_STATUS_OK) {
			return -EIO;
		}
	}

	return 0;
}

static const struct net_wifi_mgmt_offload api_funcs = {
	.wifi_api.eth_api = {
		.iface_api.init = wfx200_iface_init,
		.send = wfx200_send,
		.get_capabilities = wfx200_get_capabilities,
	},
	.scan = wfx200_scan,
	.connect = wfx200_connect,
	.disconnect = wfx200_disconnect,
	.ap_enable = wfx200_ap_enable,
	.ap_disable = wfx200_ap_disable,
};

void wfx200_interrupt_handler(const struct device *dev, struct gpio_callback *cb,
			      uint32_t pins)
{
	struct wfx200_data *context = CONTAINER_OF(cb, struct wfx200_data, int_cb);

	ARG_UNUSED(pins);
	ARG_UNUSED(dev);

	if (IS_ENABLED(CONFIG_WIFI_WFX200_BUS_SPI)) {
		if (IS_ENABLED(CONFIG_WIFI_WFX200_SLEEP)) {
			k_sem_give(&context->wakeup_sem);
		}
		k_work_submit_to_queue(&context->incoming_work_q, &context->incoming_work);
	}
}

void wfx200_incoming_work(struct k_work *item)
{
	uint16_t control_register = 0;

	ARG_UNUSED(item);

	do {
		if (sl_wfx_receive_frame(&control_register) != SL_STATUS_OK) {
			break;
		}
	} while ((control_register & SL_WFX_CONT_NEXT_LEN_MASK) != 0);
}

void wfx200_enable_interrupt(struct wfx200_data *context)
{
	const struct wfx200_config *config = context->dev->config;

	LOG_DBG("Interrupt enabled");
	if (gpio_add_callback(config->interrupt.port, &context->int_cb) < 0) {
		LOG_ERR("Failed to enable interrupt");
	}
}

void wfx200_disable_interrupt(struct wfx200_data *context)
{
	const struct wfx200_config *config = context->dev->config;

	LOG_DBG("Interrupt disabled");
	if (gpio_remove_callback(config->interrupt.port, &context->int_cb) < 0) {
		LOG_ERR("Failed to disable interrupt");
	}
}

static int wfx200_init(const struct device *dev)
{
	int res;
	const struct wfx200_config *config = dev->config;
	struct wfx200_data *context = dev->data;

	context->dev = dev;
	context->state = WFX200_STATE_IDLE;

	LOG_INF("Initializing WFX200 Driver, using FMAC Driver Version %s", FMAC_DRIVER_VERSION_STRING);

	k_heap_init(&context->heap, context->heap_buffer, CONFIG_WIFI_WFX200_HEAP_SIZE);

	if (gpio_pin_configure_dt(&config->reset,
				  GPIO_OUTPUT_INACTIVE)) {
		LOG_ERR("Couldn't configure reset pin");
		return -ENODEV;
	}

	if (gpio_pin_configure_dt(&config->interrupt,
				  GPIO_INPUT)) {
		LOG_ERR("Couldn't configure int pin");
		return -ENODEV;
	}

	if (gpio_pin_interrupt_configure_dt(&config->interrupt, GPIO_INT_EDGE_TO_ACTIVE)) {
		LOG_ERR("Failed to configure interrupt for int pin");
		return -ENODEV;
	}
	gpio_init_callback(&context->int_cb, wfx200_interrupt_handler, BIT(config->interrupt.pin));

#if ANY_INST_HAS_WAKE_GPIOS
	if (config->wakeup) {
		if (gpio_pin_configure_dt(config->wakeup,
					  GPIO_OUTPUT_ACTIVE)) {
			LOG_ERR("Couldn't configure wake pin");
			return -ENODEV;
		}
	}
#endif /* ANY_INST_HAS_wakeupWAKE_GPIOS */

#if ANY_INST_HAS_HIF_SEL_GPIOS
	if (config->hif_select) {
		if (gpio_pin_configure_dt(config->hif_select,
					  GPIO_OUTPUT_INACTIVE)) {
			LOG_ERR("Couldn't configure hif_select pin");
			return -ENODEV;
		}
	}
#endif /* ANY_INST_HAS_HIF_SEL_GPIOS */

	if (IS_ENABLED(CONFIG_WIFI_WFX200_BUS_SPI)) {
		if (!spi_is_ready(&config->bus)) {
			LOG_ERR("SPI bus %s not ready", config->bus.bus->name);
			return -ENODEV;
		}
	}

	k_work_queue_start(&context->incoming_work_q, context->wfx200_stack_area,
			   K_THREAD_STACK_SIZEOF(context->wfx200_stack_area),
			   CONFIG_WIFI_WFX200_PRIORITY,
			   &(const struct k_work_queue_config){ .name = "wfx200_rx", });
	k_thread_create(&context->event_thread, context->wfx200_event_stack_area,
			K_THREAD_STACK_SIZEOF(context->wfx200_event_stack_area),
			wfx200_event_thread, context, NULL, NULL,
			CONFIG_WIFI_WFX200_PRIORITY, 0, K_NO_WAIT);

	res = sl_wfx_init(&context->sl_context);
	if (res != SL_STATUS_OK) {
		switch (res) {
		case SL_STATUS_WIFI_INVALID_KEY:
			LOG_ERR("Failed to init WF200: Firmware keyset invalid");
			break;
		case SL_STATUS_WIFI_FIRMWARE_DOWNLOAD_TIMEOUT:
			LOG_ERR("Failed to init WF200: Firmware download timeout");
			break;
		case SL_STATUS_TIMEOUT:
			LOG_ERR("Failed to init WF200: Poll for value timeout");
			break;
		case SL_STATUS_FAIL:
			LOG_ERR("Failed to init WF200: Error");
			break;
		default:
			LOG_ERR("Failed to init WF200: Unknown error");
		}
		return -ENODEV;
	}
	LOG_INF("WFX200 driver initialized");
	LOG_INF("FW version %d.%d.%d",
		context->sl_context.firmware_major,
		context->sl_context.firmware_minor,
		context->sl_context.firmware_build);
	return 0;
}

#define INST_HAS_HIF_SEL_GPIO(idx) \
	DT_INST_NODE_HAS_PROP(idx, hif_sel_gpios)

#define INST_HIF_SEL_GPIO_SPEC(idx)				     \
	IF_ENABLED(INST_HAS_HIF_SEL_GPIO(idx),			     \
		   (static const struct gpio_dt_spec hif_sel_##idx = \
			    GPIO_DT_SPEC_INST_GET(idx, hif_sel_gpios); ))

#define INST_HAS_WAKE_GPIO(idx)	\
	DT_INST_NODE_HAS_PROP(idx, wake_gpios)

#define INST_WAKE_GPIO_SPEC(idx)				  \
	IF_ENABLED(INST_HAS_WAKE_GPIO(idx),			  \
		   (static const struct gpio_dt_spec wake_##idx = \
			    GPIO_DT_SPEC_INST_GET(idx, wake_gpios); ))

#define WIFI_WFX200_INST(idx)									 \
	static struct wfx200_data inst_##idx##_data = {						 \
		.wakeup_sem = Z_SEM_INITIALIZER(inst_##idx##_data.wakeup_sem, 0, 1),		 \
		.event_sem = Z_SEM_INITIALIZER(inst_##idx##_data.event_sem, 0, K_SEM_MAX_LIMIT), \
		.bus_mutex = Z_MUTEX_INITIALIZER(inst_##idx##_data.bus_mutex),			 \
		.event_mutex = Z_MUTEX_INITIALIZER(inst_##idx##_data.event_mutex),		 \
		.incoming_work = Z_WORK_INITIALIZER(wfx200_incoming_work),			 \
		.event_queue = Z_QUEUE_INITIALIZER(inst_##idx##_data.event_queue),		 \
	};											 \
	INST_HIF_SEL_GPIO_SPEC(idx)								 \
	INST_WAKE_GPIO_SPEC(idx)								 \
	static const char *const inst_##idx##_pds[] = DT_INST_PROP(idx, pds);			 \
	static const struct wfx200_config inst_##idx##_config = {				 \
		.bus = SPI_DT_SPEC_INST_GET(							 \
			idx, SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_OP_MODE_MASTER, 0),	 \
		.interrupt = GPIO_DT_SPEC_INST_GET(idx, int_gpios),				 \
		.reset = GPIO_DT_SPEC_INST_GET(idx, reset_gpios),				 \
		IF_ENABLED(INST_HAS_HIF_SEL_GPIO(idx),						 \
			   (.hif_select = &hif_sel_##idx, ))					 \
		IF_ENABLED(INST_HAS_WAKE_GPIO(idx),						 \
			   (.wakeup = &wake_##idx, ))						 \
		.pds = inst_##idx##_pds,							 \
		.pds_length = ARRAY_SIZE(inst_##idx##_pds),					 \
	};											 \
	struct wfx200_data *wfx200_##idx = &inst_##idx##_data;					 \
	ETH_NET_DEVICE_DT_INST_DEFINE(idx,							 \
				      wfx200_init, device_pm_control_nop,			 \
				      &inst_##idx##_data, &inst_##idx##_config,			 \
				      CONFIG_WIFI_INIT_PRIORITY, &api_funcs, NET_ETH_MTU);	 \
	BUILD_ASSERT(idx == 0, "Only one instance of the wfx200 driver is currently allowed");

DT_INST_FOREACH_STATUS_OKAY(WIFI_WFX200_INST)
