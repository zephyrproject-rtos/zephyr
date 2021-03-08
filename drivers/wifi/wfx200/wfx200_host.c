/**
 * Copyright (c) 2023 grandcentrix GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wifi_wfx200_host, CONFIG_WIFI_LOG_LEVEL);

#include <stdarg.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>

#include <sl_wfx.h>
#include "wfx200_internal.h"

/* The driver from SiLabs does use a internal global variable to store the
 * drivers context. This means, that we are limitied to one instance of SiLabs
 * driver and need to use a global variable here as well, as the host api does
 * not give a context to the callbacks. So we need to use our own global
 * variable here as well.
 */
extern struct wfx200_data *wfx200_0;

/* WFX host callbacks */
void sl_wfx_connect_callback(sl_wfx_connect_ind_t *connect);
void sl_wfx_disconnect_callback(sl_wfx_disconnect_ind_t *disconnect);
void sl_wfx_host_received_frame_callback(sl_wfx_received_ind_t *rx_buffer);
void sl_wfx_scan_result_callback(sl_wfx_scan_result_ind_t *scan_result);
void sl_wfx_scan_complete_callback(sl_wfx_scan_complete_ind_t *scan_complete);
void sl_wfx_generic_status_callback(sl_wfx_generic_ind_t *frame);
void sl_wfx_ext_auth_callback(sl_wfx_ext_auth_ind_t *ext_auth_indication);

/**
 * Functions for initializing the host, firmware uploading and pds
 */
sl_status_t sl_wfx_host_init(void)
{
	/* Initialization happens in wfx200_init, nothing to do here */
	return SL_STATUS_OK;
}

sl_status_t sl_wfx_host_get_firmware_data(const uint8_t **data, uint32_t data_size)
{
	if (data == NULL ||
	    wfx200_0->firmware_pos >= (&__sl_wfx_firmware_end - &__sl_wfx_firmware_start)) {
		return SL_STATUS_FAIL;
	}
	*data = &__sl_wfx_firmware_start + wfx200_0->firmware_pos;
	wfx200_0->firmware_pos += data_size;
	return SL_STATUS_OK;
}

sl_status_t sl_wfx_host_get_firmware_size(uint32_t *firmware_size)
{
	if (firmware_size == NULL) {
		return SL_STATUS_FAIL;
	}
	*firmware_size = (&__sl_wfx_firmware_end - &__sl_wfx_firmware_start);
	return SL_STATUS_OK;
}

sl_status_t sl_wfx_host_get_pds_data(const char **pds_data, uint16_t index)
{
	const struct wfx200_config *config = wfx200_0->dev->config;

	if (pds_data == NULL) {
		return SL_STATUS_FAIL;
	}
	*pds_data = config->pds[index];
	return SL_STATUS_OK;
}

sl_status_t sl_wfx_host_get_pds_size(uint16_t *pds_size)
{
	const struct wfx200_config *config = wfx200_0->dev->config;

	if (pds_size == NULL) {
		return SL_STATUS_FAIL;
	}
	*pds_size = config->pds_length;
	return SL_STATUS_OK;
}

sl_status_t sl_wfx_host_deinit(void)
{
	/* Nothing to do here */
	return SL_STATUS_OK;
}

/**
 * Functions for GPIO Access
 */
sl_status_t sl_wfx_host_reset_chip(void)
{
	const struct wfx200_config *config = wfx200_0->dev->config;

	LOG_DBG("resetting chip");

#if ANY_INST_HAS_HIF_SEL_GPIOS
	if (config->hif_select != NULL) {
		LOG_DBG("selecting SPI interface");
		gpio_pin_set_dt(config->hif_select, 0);
	}
#endif /* ANY_INST_HAS_HIF_SEL_GPIOS */
	gpio_pin_set_dt(&config->reset, 1);
	/* arbitrary reset pulse length */
	k_sleep(K_MSEC(50));
	gpio_pin_set_dt(&config->reset, 0);
	/* The WFM200 Wi-Fi Expansion Board has a built in reset delay of 1 ms.
	 * Wait here 2 ms to ensure the reset has been released
	 */
	k_sleep(K_MSEC(2));
	return SL_STATUS_OK;
}

sl_status_t sl_wfx_host_set_wake_up_pin(uint8_t state)
{
#if ANY_INST_HAS_WAKE_GPIOS
	const struct wfx200_config *config = wfx200_0->dev->config;

	if (config->wakeup != NULL) {
		gpio_pin_set_dt(config->wakeup, state ? 1 : 0);
	} else {
		LOG_WRN("Set wake up pin requested, but wake up pin not configured");
	}
#endif /* ANY_INST_HAS_WAKE_GPIOS */
	return SL_STATUS_OK;
}

sl_status_t sl_wfx_host_wait_for_wake_up(void)
{
	return SL_STATUS_OK;
}

sl_status_t sl_wfx_host_hold_in_reset(void)
{
	const struct wfx200_config *config = wfx200_0->dev->config;

	LOG_DBG("holding chip in reset");
	gpio_pin_set_dt(&config->reset, 1);
	return SL_STATUS_OK;
}

sl_status_t sl_wfx_host_sleep_grant(sl_wfx_host_bus_transfer_type_t type,
				    sl_wfx_register_address_t address, uint32_t length)
{
	ARG_UNUSED(type);
	ARG_UNUSED(address);
	ARG_UNUSED(length);

	return SL_STATUS_WIFI_SLEEP_NOT_GRANTED;
}

/**
 * Transmit message to the WFx chip
 */
sl_status_t sl_wfx_host_transmit_frame(void *frame, uint32_t frame_len)
{
	return sl_wfx_data_write(frame, frame_len);
}

/**
 * Functions to handle confirmations and indications
 */
sl_status_t sl_wfx_host_setup_waited_event(uint8_t event_id)
{
	k_mutex_lock(&wfx200_0->event_mutex, K_FOREVER);
	/* Resetting the semaphore will unblock all current waiting tasks.
	 * The event, they are waiting for, will never arrive.
	 */
	if (wfx200_0->waited_event_id != 0 && wfx200_0->waited_event_id != event_id) {
		LOG_WRN("Still waiting for event 0x%02x while setting up waiter for 0x%02x",
			wfx200_0->waited_event_id, event_id);
		k_sem_reset(&wfx200_0->event_sem);
	}
	wfx200_0->waited_event_id = event_id;
	k_mutex_unlock(&wfx200_0->event_mutex);
	return SL_STATUS_OK;
}

sl_status_t sl_wfx_host_wait_for_confirmation(uint8_t confirmation_id, uint32_t timeout_ms,
					      void **event_payload_out)
{
	k_mutex_lock(&wfx200_0->event_mutex, K_FOREVER);
	if (wfx200_0->waited_event_id != confirmation_id) {
		if (wfx200_0->waited_event_id == 0) {
			LOG_DBG("Confirmation waiter wasn't set up, now waiting for event 0x%02x",
				confirmation_id);
		} else {
			LOG_WRN("Confirmation waiter set up to wait for event 0x%02x but waiting "
				"for 0x%02x",
				wfx200_0->waited_event_id, confirmation_id);
		}
		k_sem_reset(&wfx200_0->event_sem);
		wfx200_0->waited_event_id = confirmation_id;
	}
	k_mutex_unlock(&wfx200_0->event_mutex);

	if (k_sem_take(&wfx200_0->event_sem, K_MSEC(timeout_ms)) == -EAGAIN) {
		LOG_WRN("Didn't receive confirmation for event 0x%02x", confirmation_id);
		return SL_STATUS_TIMEOUT;
	}
	k_mutex_lock(&wfx200_0->event_mutex, K_FOREVER);
	wfx200_0->waited_event_id = 0;
	k_mutex_unlock(&wfx200_0->event_mutex);
	if (event_payload_out != NULL) {
		*event_payload_out = wfx200_0->sl_context.event_payload_buffer;
	}
	return SL_STATUS_OK;
}

sl_status_t sl_wfx_host_wait(uint32_t wait_ms)
{
	k_sleep(K_MSEC(wait_ms));
	return SL_STATUS_OK;
}

sl_status_t sl_wfx_host_post_event(sl_wfx_generic_message_t *event_payload)
{
	union {
		sl_wfx_received_ind_t *ethernet_frame;
		sl_wfx_exception_ind_t *firmware_exception;
		sl_wfx_error_ind_t *firmware_error;
	} u;

	switch (event_payload->header.id) {
	/******** INDICATION ********/
	case SL_WFX_CONNECT_IND_ID:
		sl_wfx_connect_callback((sl_wfx_connect_ind_t *)event_payload);
		break;
	case SL_WFX_DISCONNECT_IND_ID:
		sl_wfx_disconnect_callback((sl_wfx_disconnect_ind_t *)event_payload);
		break;
	case SL_WFX_RECEIVED_IND_ID:
		u.ethernet_frame = (sl_wfx_received_ind_t *)event_payload;
		if (u.ethernet_frame->body.frame_type == 0) {
			sl_wfx_host_received_frame_callback(u.ethernet_frame);
		}
		break;
	case SL_WFX_SCAN_RESULT_IND_ID:
		sl_wfx_scan_result_callback((sl_wfx_scan_result_ind_t *)event_payload);
		break;
	case SL_WFX_SCAN_COMPLETE_IND_ID:
		sl_wfx_scan_complete_callback((sl_wfx_scan_complete_ind_t *)event_payload);
		break;
	case SL_WFX_EXT_AUTH_IND_ID:
		sl_wfx_ext_auth_callback((sl_wfx_ext_auth_ind_t *)event_payload);
		break;
	case SL_WFX_GENERIC_IND_ID:
		break;
	case SL_WFX_EXCEPTION_IND_ID:
		u.firmware_exception = (sl_wfx_exception_ind_t *)event_payload;
		LOG_ERR("Firmware exception %u", u.firmware_exception->body.reason);
		LOG_DBG("Header: id(%hhx) info(%hhx)", u.firmware_exception->header.id,
			u.firmware_exception->header.info);
		LOG_HEXDUMP_DBG(u.firmware_exception->body.data,
				u.firmware_exception->header.length -
					offsetof(sl_wfx_exception_ind_t, body.data),
				"Body data");
		break;
	case SL_WFX_ERROR_IND_ID:
		u.firmware_error = (sl_wfx_error_ind_t *)event_payload;
		LOG_ERR("Firmware error %u", u.firmware_error->body.type);
		LOG_DBG("Header: id(%hhx) info(%hhx)", u.firmware_error->header.id,
			u.firmware_error->header.info);
		LOG_HEXDUMP_DBG(u.firmware_error->body.data,
				u.firmware_error->header.length -
					offsetof(sl_wfx_error_ind_t, body.data),
				"Body data");
		break;
	}

	k_mutex_lock(&wfx200_0->event_mutex, K_FOREVER);
	if (wfx200_0->waited_event_id == event_payload->header.id) {
		if (event_payload->header.length <
		    sizeof(wfx200_0->sl_context.event_payload_buffer)) {
			/* Post the event into the "queue".
			 * Not using a queue here. The SiLabs examples use the queue similar to a
			 * counting semaphore and are not passing the "queue" elements via the
			 * "queue" but instead with a single buffer. So we can only communicate a
			 * single element at the same time. The sample code also only adds elements
			 * to the "queue", which it has been configured to explicit waiting for. A
			 * queue is therefore not needed.
			 */
			memcpy(wfx200_0->sl_context.event_payload_buffer, (void *)event_payload,
			       event_payload->header.length);
			k_sem_give(&wfx200_0->event_sem);
		}
	}
	k_mutex_unlock(&wfx200_0->event_mutex);

	return SL_STATUS_OK;
}

/* WFX host callbacks */
void sl_wfx_connect_callback(sl_wfx_connect_ind_t *connect)
{
	struct wfx200_queue_event *event =
		k_heap_alloc(&wfx200_0->heap, sizeof(struct wfx200_queue_event), K_NO_WAIT);

	if (event == NULL) {
		/* TODO */
		return;
	}

	switch (connect->body.status) {
	case WFM_STATUS_SUCCESS:
		LOG_INF("Connected");
		wfx200_0->sl_context.state |= SL_WFX_STA_INTERFACE_CONNECTED;
		event->ev = WFX200_CONNECT_EVENT;
		k_queue_append(&wfx200_0->event_queue, event);
		return;
	case WFM_STATUS_NO_MATCHING_AP:
		LOG_ERR("Connection failed, access point not found");
		break;
	case WFM_STATUS_CONNECTION_ABORTED:
		LOG_ERR("Connection aborted");
		break;
	case WFM_STATUS_CONNECTION_TIMEOUT:
		LOG_ERR("Connection timeout");
		break;
	case WFM_STATUS_CONNECTION_REJECTED_BY_AP:
		LOG_ERR("Connection rejected by the access point");
		break;
	case WFM_STATUS_CONNECTION_AUTH_FAILURE:
		LOG_ERR("Connection authentication failure");
		break;
	default:
		LOG_ERR("Connection attempt error");
	}
	event->ev = WFX200_CONNECT_FAILED_EVENT;
	k_queue_append(&wfx200_0->event_queue, event);
}

void sl_wfx_disconnect_callback(sl_wfx_disconnect_ind_t *disconnect)
{
	ARG_UNUSED(disconnect);
	struct wfx200_queue_event *event =
		k_heap_alloc(&wfx200_0->heap, sizeof(struct wfx200_queue_event), K_NO_WAIT);

	if (event == NULL) {
		/* TODO */
		return;
	}

	wfx200_0->sl_context.state &= ~SL_WFX_STA_INTERFACE_CONNECTED;
	event->ev = WFX200_DISCONNECT_EVENT;
	k_queue_append(&wfx200_0->event_queue, event);

	LOG_INF("WiFi disconnected");
}

void sl_wfx_host_received_frame_callback(sl_wfx_received_ind_t *rx_buffer)
{
	if ((rx_buffer->header.info & SL_WFX_MSG_INFO_INTERFACE_MASK) !=
	    (SL_WFX_STA_INTERFACE << SL_WFX_MSG_INFO_INTERFACE_OFFSET)) {
		LOG_ERR("Received spurious ethernet frame");
		return;
	}

	if (wfx200_0->iface == NULL) {
		LOG_ERR("Network device unavailable");
		return;
	}

	struct net_pkt *pkt = net_pkt_rx_alloc_with_buffer(
		wfx200_0->iface, rx_buffer->body.frame_length, AF_UNSPEC, 0, K_NO_WAIT);
	if (!pkt) {
		LOG_ERR("Failed to get net buffer");
		return;
	}
	if (net_pkt_write(pkt, ((uint8_t *)rx_buffer->body.frame) + rx_buffer->body.frame_padding,
			  rx_buffer->body.frame_length) < 0) {
		LOG_ERR("Failed to write pkt");
		goto pkt_unref;
	}

	int res = net_recv_data(wfx200_0->iface, pkt);

	if (res < 0) {
		LOG_ERR("Failed to push received data %d", res);
		goto pkt_unref;
	}
	return;

pkt_unref:
	net_pkt_unref(pkt);
}

void sl_wfx_scan_result_callback(sl_wfx_scan_result_ind_t *scan_result)
{
	struct sl_wfx_scan_result_ind_body_s *body = &scan_result->body;
	int rssi = (body->rcpi / 2) - 110;
	struct wifi_scan_result wifi_scan_result = {0};

	if (wfx200_0->iface == NULL || wfx200_0->scan_cb == NULL) {
		return;
	}

	memcpy(wifi_scan_result.ssid, body->ssid_def.ssid,
	       MIN(sizeof(wifi_scan_result.ssid), body->ssid_def.ssid_length));
	wifi_scan_result.ssid_length = body->ssid_def.ssid_length;
	wifi_scan_result.channel = body->channel;
	wifi_scan_result.rssi = rssi;

	if (body->security_mode.wep) {
		wifi_scan_result.security = WIFI_SECURITY_TYPE_WEP;
	} else if (body->security_mode.eap) {
		wifi_scan_result.security = WIFI_SECURITY_TYPE_EAP;
	} else if (body->security_mode.psk) {
		if (body->security_mode.wpa3) {
			wifi_scan_result.security = WIFI_SECURITY_TYPE_SAE;
		} else if (body->security_mode.wpa2) {
			wifi_scan_result.security = WIFI_SECURITY_TYPE_PSK;
		} else if (body->security_mode.wpa) {
			wifi_scan_result.security = WIFI_SECURITY_TYPE_WPA_PSK;
		} else {
			wifi_scan_result.security = WIFI_SECURITY_TYPE_UNKNOWN;
		}
	} else {
		wifi_scan_result.security = WIFI_SECURITY_TYPE_NONE;
	}

	wifi_scan_result.mfp = (body->security_mode.pmf) ? WIFI_MFP_REQUIRED : WIFI_MFP_DISABLE;

	memcpy(wifi_scan_result.mac, body->mac, sizeof(wifi_scan_result.mac));
	wifi_scan_result.mac_length = sizeof(wifi_scan_result.mac);
	/* WF200 does only support 2.4 GHz */
	wifi_scan_result.band = WIFI_FREQ_BAND_2_4_GHZ;

	wfx200_0->scan_cb(wfx200_0->iface, 0, &wifi_scan_result);
}

void sl_wfx_scan_complete_callback(sl_wfx_scan_complete_ind_t *scan_complete)
{
	ARG_UNUSED(scan_complete);

	if (wfx200_0->iface == NULL || wfx200_0->scan_cb == NULL) {
		return;
	}

	if (scan_complete->body.status == 0) {
		LOG_DBG("Scan complete");
		if (wfx200_0->scan_cb != NULL) {
			wfx200_0->scan_cb(wfx200_0->iface, 0, NULL);
		}
	} else {
		LOG_WRN("Scan failed(%d)", scan_complete->body.status);
		if (wfx200_0->scan_cb != NULL) {
			wfx200_0->scan_cb(wfx200_0->iface, 1, NULL);
		}
	}
	wfx200_0->scan_cb = NULL;
}

void sl_wfx_generic_status_callback(sl_wfx_generic_ind_t *frame)
{
	ARG_UNUSED(frame);
	LOG_DBG("%s", __func__);
}

void sl_wfx_ext_auth_callback(sl_wfx_ext_auth_ind_t *ext_auth_indication)
{
	ARG_UNUSED(ext_auth_indication);
	LOG_DBG("%s", __func__);
}

sl_status_t sl_wfx_host_allocate_buffer(void **buffer, sl_wfx_buffer_type_t type,
					uint32_t buffer_size)
{
	ARG_UNUSED(type);

	if (buffer == NULL) {
		return SL_STATUS_FAIL;
	}
	*buffer = k_heap_alloc(&wfx200_0->heap, buffer_size, K_NO_WAIT);
	if (*buffer == NULL) {
		LOG_ERR("Failed to allocate %d bytes", buffer_size);
		return SL_STATUS_ALLOCATION_FAILED;
	}
	return SL_STATUS_OK;
}

sl_status_t sl_wfx_host_free_buffer(void *buffer, sl_wfx_buffer_type_t type)
{
	ARG_UNUSED(type);

	if (buffer != NULL) {
		k_heap_free(&wfx200_0->heap, buffer);
	}
	return SL_STATUS_OK;
}

/**
 * Bus access
 */
sl_status_t sl_wfx_host_lock(void)
{
	/* Time taken from silabs sample code */
	if (k_mutex_lock(&wfx200_0->bus_mutex, K_MSEC(500)) == -EAGAIN) {
		LOG_DBG("Wi-Fi driver mutex timeout");
		return SL_STATUS_TIMEOUT;
	}
	return SL_STATUS_OK;
}

sl_status_t sl_wfx_host_unlock(void)
{
	k_mutex_unlock(&wfx200_0->bus_mutex);
	return SL_STATUS_OK;
}

/* WF200 host bus API */
sl_status_t sl_wfx_host_init_bus(void)
{
	/* Initialization happens in wfx200_init, nothing to do here */
	return SL_STATUS_OK;
}

sl_status_t sl_wfx_host_deinit_bus(void)
{
	/* Nothing to do here */
	return SL_STATUS_OK;
}

sl_status_t sl_wfx_host_enable_platform_interrupt(void)
{
	wfx200_enable_interrupt(wfx200_0);
	return SL_STATUS_OK;
}

sl_status_t sl_wfx_host_disable_platform_interrupt(void)

{
	wfx200_disable_interrupt(wfx200_0);
	return SL_STATUS_OK;
}

/* WF200 host SPI bus API */
sl_status_t sl_wfx_host_spi_cs_assert(void)
{
	/* Not needed, cs will be handled by spi driver */
	return SL_STATUS_OK;
}

sl_status_t sl_wfx_host_spi_cs_deassert(void)
{
	/* Not needed, cs will be handled by spi driver */
	return SL_STATUS_OK;
}

/* Despite its name cs will be handled by spi hardware */
sl_status_t sl_wfx_host_spi_transfer_no_cs_assert(sl_wfx_host_bus_transfer_type_t type,
						  uint8_t *header, uint16_t header_length,
						  uint8_t *buffer, uint16_t buffer_length)
{
	const struct wfx200_config *config = wfx200_0->dev->config;
	int err;
	const struct spi_buf_set tx = {
		.buffers =
			(const struct spi_buf[]){
				{.buf = header, .len = header_length},
				{.buf = buffer, .len = buffer_length},
			},
		.count = (type == SL_WFX_BUS_WRITE) ? 2 : 1,
	};
	const struct spi_buf_set rx = {
		.buffers =
			(const struct spi_buf[]){
				{.buf = NULL, .len = header_length},
				{.buf = buffer, .len = buffer_length},
			},
		.count = 2,
	};

	if (header == NULL || buffer == NULL) {
		return SL_STATUS_FAIL;
	}
	if (type == SL_WFX_BUS_WRITE) {
		err = spi_write_dt(&config->bus, &tx);
		if (err < 0) {
			LOG_ERR("spi_write fail: %d", err);
			return SL_STATUS_FAIL;
		}
	} else {
		err = spi_transceive_dt(&config->bus, &tx, &rx);
		if (err < 0) {
			LOG_ERR("spi_transceive fail: %d", err);
			return SL_STATUS_FAIL;
		}
	}
	return SL_STATUS_OK;
}

#define TRUNCATED_MESSAGE "<truncated>"
BUILD_ASSERT(sizeof(TRUNCATED_MESSAGE) < CONFIG_WIFI_WFX200_LOG_BUFFER_SIZE,
	     "Log buffer must allow \"" TRUNCATED_MESSAGE "\" message");

void sl_wfx_host_log(const char *string, ...)
{
	if (IS_ENABLED(CONFIG_WIFI_WFX200_VERBOSE_LOGGING)) {
		va_list args;
		static char log_buffer[CONFIG_WIFI_WFX200_LOG_BUFFER_SIZE];
		int ret;

		va_start(args, string);
		ret = vsnprintf(log_buffer, sizeof(log_buffer), string, args);
		if (ret >= sizeof(log_buffer)) {
			memcpy(log_buffer + sizeof(log_buffer) - sizeof(TRUNCATED_MESSAGE),
			       TRUNCATED_MESSAGE, sizeof(TRUNCATED_MESSAGE));
		}
		log_buffer[sizeof(log_buffer) - 1] = 0;
		LOG_DBG("wfx-fullmac-driver: %s", log_buffer);
		va_end(args);
	}
}
