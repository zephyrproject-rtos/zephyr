/**
 * SiLabs WFX200 WiFi Module with SPI Interface
 *
 * Copyright (c) 2023 grandcentrix GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wifi_wfx200, CONFIG_WIFI_LOG_LEVEL);

#include <string.h>
#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/device.h>

#include <sl_wfx.h>
#include "wfx200_internal.h"

static int wifi_security_to_wfx(enum wifi_security_type security, sl_wfx_security_mode_t *mode)
{
	switch (security) {
	case WIFI_SECURITY_TYPE_SAE:
		*mode = WFM_SECURITY_MODE_WPA3_SAE;
		break;
	case WIFI_SECURITY_TYPE_PSK:
		*mode = WFM_SECURITY_MODE_WPA2_PSK;
		break;
	case WIFI_SECURITY_TYPE_WPA_PSK:
		*mode = WFM_SECURITY_MODE_WPA2_WPA1_PSK;
		break;
	case WIFI_SECURITY_TYPE_WEP:
		*mode = WFM_SECURITY_MODE_WEP;
		break;
	case WIFI_SECURITY_TYPE_NONE:
		*mode = WFM_SECURITY_MODE_OPEN;
		break;
	default:
		LOG_ERR("Not supported security mode: %s", wifi_security_txt(security));
		return -ENOTSUP;
	}

	return 0;
}

static int wifi_mfp_to_wfx(enum wifi_mfp_options mfp, uint16_t *out)
{
	switch (mfp) {
	case WIFI_MFP_DISABLE:
		*out = WFM_MGMT_FRAME_PROTECTION_DISABLED;
		break;
	case WIFI_MFP_OPTIONAL:
		*out = WFM_MGMT_FRAME_PROTECTION_OPTIONAL;
		break;
	case WIFI_MFP_REQUIRED:
		*out = WFM_MGMT_FRAME_PROTECTION_MANDATORY;
		break;
	default:
		LOG_ERR("Not supported management frame option: %s", wifi_mfp_txt(mfp));
		return -EINVAL;
	}

	return 0;
}

static void wfx200_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct wfx200_data *data = dev->data;

	struct ethernet_context *eth_ctx = net_if_l2_data(iface);

	LOG_DBG("Interface init");

	eth_ctx->eth_if_type = L2_ETH_IF_TYPE_WIFI;

	ethernet_init(iface);
	net_if_carrier_off(iface);

	data->iface = iface;
	net_if_set_link_addr(iface, data->sl_context.mac_addr_0.octet,
			     sizeof(data->sl_context.mac_addr_0.octet), NET_LINK_ETHERNET);
}

int wfx200_send(const struct device *dev, struct net_pkt *pkt)
{
	struct wfx200_data *data = dev->data;
	size_t len = net_pkt_get_len(pkt), frame_len;
	sl_wfx_send_frame_req_t *tx_buffer;
	sl_status_t result;
	int status = 0;

	if (!data->iface) {
		status = -ENODEV;
		goto out;
	}

	if (!(data->sl_context.state & SL_WFX_STA_INTERFACE_CONNECTED)) {
		status = -EIO;
		goto out;
	}

	frame_len = SL_WFX_ROUND_UP(len, 2);

	result = sl_wfx_allocate_command_buffer((sl_wfx_generic_message_t **)&tx_buffer,
						SL_WFX_SEND_FRAME_REQ_ID, SL_WFX_TX_FRAME_BUFFER,
						frame_len + sizeof(sl_wfx_send_frame_req_t));
	if (result != SL_STATUS_OK) {
		status = -ENOMEM;
		goto out;
	}

	if (net_pkt_read(pkt, tx_buffer->body.packet_data, len)) {
		status = -EIO;
		goto out_free;
	}

	result = sl_wfx_send_ethernet_frame(tx_buffer, frame_len, SL_WFX_STA_INTERFACE,
					    WFM_PRIORITY_BE0);
	if (result != SL_STATUS_OK) {
		status = -EIO;
		goto out_free;
	}

out_free:
	sl_wfx_free_command_buffer((sl_wfx_generic_message_t *)tx_buffer, SL_WFX_SEND_FRAME_REQ_ID,
				   SL_WFX_TX_FRAME_BUFFER);
out:
	return status;
}

static enum ethernet_hw_caps wfx200_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int wfx200_scan(const struct device *dev, struct wifi_scan_params *params,
		       scan_result_cb_t cb)
{
	struct wfx200_data *data = dev->data;
	sl_status_t result;

	if (data->scan_cb != NULL) {
		return -EINPROGRESS;
	}

	uint16_t scan_mode = (params->scan_type == WIFI_SCAN_TYPE_ACTIVE) ? WFM_SCAN_MODE_ACTIVE
									  : WFM_SCAN_MODE_PASSIVE;

	data->scan_cb = cb;
	result = sl_wfx_send_scan_command(scan_mode, NULL, 0, NULL, 0, NULL, 0, NULL);
	if (result != SL_STATUS_OK) {
		data->scan_cb = NULL;
		return -EIO;
	}

	return 0;
}

static int wfx200_connect(const struct device *dev, struct wifi_connect_req_params *params)
{
	struct wfx200_data *data = dev->data;
	sl_status_t result;

	if (params->band != WIFI_FREQ_BAND_2_4_GHZ) {
		return -ENOTSUP;
	}

	if (params->security != WIFI_SECURITY_TYPE_NONE) {
		if (params->psk == NULL || params->psk_length > SL_WFX_PASSWORD_SIZE) {
			return -EINVAL;
		}
	}

	/* The Station interface should have been initialized before connecting the Station
	 * anywhere.
	 */
	if (!data->iface) {
		return -ENODEV;
	}

	if (data->sta_enabled) {
		return -EALREADY;
	}

	char ssid[WIFI_SSID_MAX_LEN + 1];

	memcpy(ssid, params->ssid, MIN(params->ssid_length, sizeof(ssid)));
	ssid[MIN(params->ssid_length, WIFI_SSID_MAX_LEN)] = 0;

	sl_wfx_security_mode_t mode;
	int res = wifi_security_to_wfx(params->security, &mode);

	if (res < 0) {
		return res;
	}

	uint16_t mfp;

	res = wifi_mfp_to_wfx(params->mfp, &mfp);
	if (res < 0) {
		return res;
	}

	uint16_t channel;

	if (params->channel <= WIFI_CHANNEL_MAX) {
		channel = params->channel;
	} else {
		channel = 0;
	}

	LOG_INF("Connecting to %s on channel %d", ssid, channel);

	result = sl_wfx_send_join_command(params->ssid, params->ssid_length, NULL, channel, mode, 0,
					  mfp, (const uint8_t *)params->psk, params->psk_length,
					  NULL, 0);
	if (result != SL_STATUS_OK) {
		LOG_DBG("Failed to send join command: %d", result);
		return -EIO;
	}
	data->sta_enabled = true;

	return 0;
}

static int wfx200_disconnect(const struct device *dev)
{
	struct wfx200_data *data = dev->data;
	sl_status_t result;

	/* The Station interface should have been initialized before connecting the Station
	 * anywhere.
	 */
	if (!data->iface) {
		return -ENODEV;
	}

	if (data->sl_context.state & SL_WFX_STA_INTERFACE_CONNECTED) {
		result = sl_wfx_send_disconnect_command();
		if (result != SL_STATUS_OK) {
			return -EIO;
		}
	}

	return 0;
}

static const struct wifi_mgmt_ops wifi_mgmt = {
	.scan = wfx200_scan,
	.connect = wfx200_connect,
	.disconnect = wfx200_disconnect,
};

static const struct net_wifi_mgmt_offload api_funcs = {
	.wifi_iface = {
			.iface_api.init = wfx200_iface_init,
			.send = wfx200_send,
			.get_capabilities = wfx200_get_capabilities,
		},
	.wifi_mgmt_api = &wifi_mgmt,
};

void wfx200_interrupt_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct wfx200_data *data = CONTAINER_OF(cb, struct wfx200_data, int_cb);

	ARG_UNUSED(pins);
	ARG_UNUSED(dev);

	k_work_submit_to_queue(&data->incoming_work_q, &data->incoming_work);
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
	if (gpio_add_callback_dt(&config->interrupt, &context->int_cb) < 0) {
		LOG_ERR("Failed to enable interrupt");
	}
}

void wfx200_disable_interrupt(struct wfx200_data *context)
{
	const struct wfx200_config *config = context->dev->config;

	LOG_DBG("Interrupt disabled");
	if (gpio_remove_callback_dt(&config->interrupt, &context->int_cb) < 0) {
		LOG_ERR("Failed to disable interrupt");
	}
}

static int wfx200_init(const struct device *dev)
{
	int res;
	const struct wfx200_config *config = dev->config;
	struct wfx200_data *data = dev->data;

	data->dev = dev;

	LOG_INF("Initializing WFX200 Driver, using FMAC Driver Version %s",
		FMAC_DRIVER_VERSION_STRING);

	k_heap_init(&data->heap, data->heap_buffer, CONFIG_WIFI_WFX200_HEAP_SIZE);

	if (gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT_INACTIVE)) {
		LOG_ERR("Couldn't configure reset pin");
		return -ENODEV;
	}

	if (gpio_pin_configure_dt(&config->interrupt, GPIO_INPUT)) {
		LOG_ERR("Couldn't configure int pin");
		return -ENODEV;
	}

	if (gpio_pin_interrupt_configure_dt(&config->interrupt, GPIO_INT_EDGE_TO_ACTIVE)) {
		LOG_ERR("Failed to configure interrupt for int pin");
		return -ENODEV;
	}
	gpio_init_callback(&data->int_cb, wfx200_interrupt_handler, BIT(config->interrupt.pin));

#if HAS_WAKE_GPIO
	if (gpio_pin_configure_dt(&config->wakeup, GPIO_OUTPUT_ACTIVE)) {
		LOG_ERR("Couldn't configure wake pin");
		return -ENODEV;
	}
#endif /* HAS_WAKE_GPIO */

#if HAS_HIF_SEL_GPIO
	if (gpio_pin_configure_dt(&config->hif_select, GPIO_OUTPUT_INACTIVE)) {
		LOG_ERR("Couldn't configure hif_select pin");
		return -ENODEV;
	}
#endif /* HAS_HIF_SEL_GPIO */

	if (!device_is_ready(config->bus.bus)) {
		LOG_ERR("SPI bus %s not ready", config->bus.bus->name);
		return -ENODEV;
	}

	k_work_queue_start(&data->incoming_work_q, data->wfx200_stack_area,
			   K_THREAD_STACK_SIZEOF(data->wfx200_stack_area),
			   CONFIG_WIFI_WFX200_PRIORITY,
			   &(const struct k_work_queue_config){
				   .name = "wfx200_rx",
			   });
	k_thread_create(&data->event_thread, data->wfx200_event_stack_area,
			K_THREAD_STACK_SIZEOF(data->wfx200_event_stack_area), wfx200_event_thread,
			data, NULL, NULL, CONFIG_WIFI_WFX200_PRIORITY, 0, K_NO_WAIT);

	res = sl_wfx_init(&data->sl_context);
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
	LOG_INF("FW version %d.%d.%d", data->sl_context.firmware_major,
		data->sl_context.firmware_minor, data->sl_context.firmware_build);
	return 0;
}

static struct wfx200_data wfx200_data = {
	.event_sem = Z_SEM_INITIALIZER(wfx200_data.event_sem, 0, K_SEM_MAX_LIMIT),
	.bus_mutex = Z_MUTEX_INITIALIZER(wfx200_data.bus_mutex),
	.event_mutex = Z_MUTEX_INITIALIZER(wfx200_data.event_mutex),
	.incoming_work = Z_WORK_INITIALIZER(wfx200_incoming_work),
	.event_queue = Z_QUEUE_INITIALIZER(wfx200_data.event_queue),
};

static const char *const wfx200_pds[] = DT_INST_PROP(0, pds);
static const struct wfx200_config wfx200_config = {
	.bus = SPI_DT_SPEC_INST_GET(0, SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_OP_MODE_MASTER, 0),
	.interrupt = GPIO_DT_SPEC_INST_GET(0, int_gpios),
	.reset = GPIO_DT_SPEC_INST_GET(0, reset_gpios),
#if HAS_HIF_SEL_GPIO
	.hif_select = GPIO_DT_SPEC_INST_GET(0, hif_sel_gpios),
#endif
#if HAS_WAKE_GPIO
	.wakeup = GPIO_DT_SPEC_INST_GET(0, wake_gpios),
#endif
	.pds = wfx200_pds,
	.pds_length = ARRAY_SIZE(wfx200_pds),
};

/* For use in wfx200_host.c */
struct wfx200_data *wfx200_0 = &wfx200_data;

NET_DEVICE_DT_INST_DEFINE(0, wfx200_init, device_pm_control_nop, &wfx200_data, &wfx200_config,
			  CONFIG_WIFI_INIT_PRIORITY, &api_funcs, ETHERNET_L2,
			  NET_L2_GET_CTX_TYPE(ETHERNET_L2), NET_ETH_MTU);
