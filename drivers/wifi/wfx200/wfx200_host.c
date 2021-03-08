/**
 * Copyright (c) 2022 grandcentrix GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME wifi_wfx200_host
#define LOG_LEVEL CONFIG_WIFI_LOG_LEVEL

#include <sl_wfx.h>
#undef BIT

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdarg.h>
#include <stdint.h>

#include <zephyr.h>
#include <device.h>
#include <sys/util.h>
#include <drivers/spi.h>
#include <drivers/gpio.h>

#include "wfx200_internal.h"

#include <sl_wfx_wf200_C0.h>

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
void sl_wfx_start_ap_callback(sl_wfx_start_ap_ind_t *start_ap);
void sl_wfx_stop_ap_callback(sl_wfx_stop_ap_ind_t *stop_ap);
void sl_wfx_host_received_frame_callback(sl_wfx_received_ind_t *rx_buffer);
void sl_wfx_scan_result_callback(sl_wfx_scan_result_ind_t *scan_result);
void sl_wfx_scan_complete_callback(sl_wfx_scan_complete_ind_t *scan_complete);
void sl_wfx_generic_status_callback(sl_wfx_generic_ind_t *frame);
void sl_wfx_ap_client_connected_callback(sl_wfx_ap_client_connected_ind_t *ap_client_connected);
void sl_wfx_ap_client_rejected_callback(sl_wfx_ap_client_rejected_ind_t *ap_client_rejected);
void sl_wfx_ap_client_disconnected_callback(sl_wfx_ap_client_disconnected_ind_t *ap_client_disconnected);
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
	if (data == NULL || wfx200_0->firmware_pos >= sizeof(sl_wfx_firmware)) {
		return SL_STATUS_FAIL;
	}
	*data = sl_wfx_firmware + wfx200_0->firmware_pos;
	wfx200_0->firmware_pos += data_size;
	return SL_STATUS_OK;
}

sl_status_t sl_wfx_host_get_firmware_size(uint32_t *firmware_size)
{
	if (firmware_size == NULL) {
		return SL_STATUS_FAIL;
	}
	*firmware_size = sizeof(sl_wfx_firmware);
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
	const struct wfx200_config *config = wfx200_0->dev->config;

#if ANY_INST_HAS_WAKE_GPIOS
	if (config->wakeup != NULL) {
		gpio_pin_set_dt(config->wakeup, state?1:0);
	} else {
		LOG_WRN("Set wake up pin requested, but wake up pin not configured");
	}
#endif /* ANY_INST_HAS_WAKE_GPIOS */
	return SL_STATUS_OK;
}

sl_status_t sl_wfx_host_wait_for_wake_up(void)
{
	int result;

	if (IS_ENABLED(CONFIG_WIFI_WFX200_SLEEP)) {
		k_sem_take(&wfx200_0->wakeup_sem, K_NO_WAIT);
		/* Time taken from sample code */
		result = k_sem_take(&wfx200_0->wakeup_sem, K_MSEC(3));
		if (result == -EAGAIN) {
			LOG_DBG("Wake up timed out");
			return SL_STATUS_TIMEOUT;
		}
	}
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
				    sl_wfx_register_address_t address,
				    uint32_t length)
{
	ARG_UNUSED(type);
	ARG_UNUSED(address);
	ARG_UNUSED(length);
	/* TODO: we could give a answer depending on the host devices state */
	if (IS_ENABLED(CONFIG_WIFI_WFX200_SLEEP)) {
		return SL_STATUS_WIFI_SLEEP_GRANTED;
	}
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
	 * The event, they are waiting for, will never arrive.*/
	if (wfx200_0->waited_event_id != 0 && wfx200_0->waited_event_id != event_id) {
		LOG_WRN("Still waiting for event 0x%02x while setting up waiter for 0x%02x",
			wfx200_0->waited_event_id, event_id);
		k_sem_reset(&wfx200_0->event_sem);
	}
	wfx200_0->waited_event_id = event_id;
	k_mutex_unlock(&wfx200_0->event_mutex);
	return SL_STATUS_OK;
}

sl_status_t sl_wfx_host_wait_for_confirmation(uint8_t confirmation_id,
					      uint32_t timeout_ms,
					      void **event_payload_out)
{
	k_mutex_lock(&wfx200_0->event_mutex, K_FOREVER);
	if (wfx200_0->waited_event_id != confirmation_id) {
		if (wfx200_0->waited_event_id == 0) {
			LOG_DBG("Confirmation waiter wasn't set up, now waiting for event 0x%02x",
				confirmation_id);
		} else {
			LOG_WRN("Confirmation waiter set up to wait for event 0x%02x but waiting for 0x%02x",
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
		sl_wfx_connect_callback((sl_wfx_connect_ind_t *) event_payload);
		break;
	case SL_WFX_DISCONNECT_IND_ID:
		sl_wfx_disconnect_callback((sl_wfx_disconnect_ind_t *) event_payload);
		break;
	case SL_WFX_START_AP_IND_ID:
		sl_wfx_start_ap_callback((sl_wfx_start_ap_ind_t *) event_payload);
		break;
	case SL_WFX_STOP_AP_IND_ID:
		sl_wfx_stop_ap_callback((sl_wfx_stop_ap_ind_t *) event_payload);
		break;
	case SL_WFX_RECEIVED_IND_ID:
		u.ethernet_frame = (sl_wfx_received_ind_t *) event_payload;
		if (u.ethernet_frame->body.frame_type == 0) {
			sl_wfx_host_received_frame_callback(u.ethernet_frame);
		}
		break;
	case SL_WFX_SCAN_RESULT_IND_ID:
		sl_wfx_scan_result_callback((sl_wfx_scan_result_ind_t *) event_payload);
		break;
	case SL_WFX_SCAN_COMPLETE_IND_ID:
		sl_wfx_scan_complete_callback((sl_wfx_scan_complete_ind_t *) event_payload);
		break;
	case SL_WFX_AP_CLIENT_CONNECTED_IND_ID:
		sl_wfx_ap_client_connected_callback((sl_wfx_ap_client_connected_ind_t *) event_payload);
		break;
	case SL_WFX_AP_CLIENT_REJECTED_IND_ID:
		sl_wfx_ap_client_rejected_callback((sl_wfx_ap_client_rejected_ind_t *) event_payload);
		break;
	case SL_WFX_AP_CLIENT_DISCONNECTED_IND_ID:
		sl_wfx_ap_client_disconnected_callback(
			(sl_wfx_ap_client_disconnected_ind_t *) event_payload);
		break;
	case SL_WFX_EXT_AUTH_IND_ID:
		sl_wfx_ext_auth_callback((sl_wfx_ext_auth_ind_t *) event_payload);
		break;
	case SL_WFX_GENERIC_IND_ID:
		break;
	case SL_WFX_EXCEPTION_IND_ID:
		u.firmware_exception = (sl_wfx_exception_ind_t *)event_payload;
		LOG_ERR("Firmware exception %u", u.firmware_exception->body.reason);
		LOG_DBG("Header: id(%hhx) info(%hhx)", u.firmware_exception->header.id,
			u.firmware_exception->header.info);
		LOG_HEXDUMP_DBG(u.firmware_exception->body.data,
				u.firmware_exception->header.length - offsetof(sl_wfx_exception_ind_t, body.data),
				"Body data");
		break;
	case SL_WFX_ERROR_IND_ID:
		u.firmware_error = (sl_wfx_error_ind_t *)event_payload;
		LOG_ERR("Firmware error %u", u.firmware_error->body.type);
		LOG_DBG("Header: id(%hhx) info(%hhx)", u.firmware_error->header.id,
			u.firmware_error->header.info);
		LOG_HEXDUMP_DBG(u.firmware_error->body.data,
				u.firmware_error->header.length - offsetof(sl_wfx_error_ind_t, body.data),
				"Body data");
		break;
	}

	k_mutex_lock(&wfx200_0->event_mutex, K_FOREVER);
	if (wfx200_0->waited_event_id == event_payload->header.id) {
		if (event_payload->header.length < sizeof(wfx200_0->sl_context.event_payload_buffer)) {
			/* Post the event into the "queue".
			 * Not using a queue here. The SiLabs examples use the queue similar to a
			 * counting semaphore and are not passing the "queue" elements via the "queue"
			 * but instead with a single buffer. So we can only communicate a single
			 * element at the same time. The sample code also only adds elements to the "queue",
			 * which it has been configured to explicit waiting for. A queue is therefore
			 * not needed.
			 * */
			memcpy(wfx200_0->sl_context.event_payload_buffer,
			       (void *) event_payload,
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
	struct wfx200_queue_event *event = k_heap_alloc(&wfx200_0->heap,
							sizeof(struct wfx200_queue_event), K_NO_WAIT);

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
	struct wfx200_queue_event *event = k_heap_alloc(&wfx200_0->heap,
							sizeof(struct wfx200_queue_event), K_NO_WAIT);

	if (event == NULL) {
		/* TODO */
		return;
	}

	wfx200_0->sl_context.state &= ~SL_WFX_STA_INTERFACE_CONNECTED;
	event->ev = WFX200_DISCONNECT_EVENT;
	k_queue_append(&wfx200_0->event_queue, event);

	LOG_INF("WiFi disconnected");
}

void sl_wfx_start_ap_callback(sl_wfx_start_ap_ind_t *start_ap)
{
	struct wfx200_queue_event *event = k_heap_alloc(&wfx200_0->heap,
							sizeof(struct wfx200_queue_event), K_NO_WAIT);

	if (event == NULL) {
		/* TODO */
		return;
	}

	if (start_ap->body.status == 0) {
		LOG_INF("AP started");
		wfx200_0->sl_context.state |= SL_WFX_AP_INTERFACE_UP;
		event->ev = WFX200_AP_START_EVENT;
	} else {
		LOG_ERR("AP start failed(%d)", start_ap->body.status);
		event->ev = WFX200_AP_START_FAILED_EVENT;
	}
	k_queue_append(&wfx200_0->event_queue, event);
}

void sl_wfx_stop_ap_callback(sl_wfx_stop_ap_ind_t *stop_ap)
{
	struct wfx200_queue_event *event = k_heap_alloc(&wfx200_0->heap,
							sizeof(struct wfx200_queue_event), K_NO_WAIT);

	ARG_UNUSED(stop_ap);

	if (event == NULL) {
		/* TODO */
		return;
	}

	LOG_DBG("AP Stopped");
	wfx200_0->sl_context.state &= ~SL_WFX_AP_INTERFACE_UP;
	event->ev = WFX200_AP_STOP_EVENT;
	k_queue_append(&wfx200_0->event_queue, event);
}

void sl_wfx_host_received_frame_callback(sl_wfx_received_ind_t *rx_buffer)
{
	struct net_pkt *pkt;
	int res;

	if (wfx200_0->iface == NULL) {
		LOG_ERR("Network interface unavailable");
		return;
	}
	switch (wfx200_0->state) {
	case WFX200_STATE_STA_MODE:
		if ((rx_buffer->header.info & SL_WFX_MSG_INFO_INTERFACE_MASK) ==
		    (SL_WFX_SOFTAP_INTERFACE << SL_WFX_MSG_INFO_INTERFACE_OFFSET)) {
			LOG_WRN("Got ethernet packet from softap interface in sta mode. Dropping packet...");
			return;
		}
		break;
	case WFX200_STATE_AP_MODE:
		if ((rx_buffer->header.info & SL_WFX_MSG_INFO_INTERFACE_MASK) ==
		    (SL_WFX_STA_INTERFACE << SL_WFX_MSG_INFO_INTERFACE_OFFSET)) {
			LOG_WRN("Got ethernet packet from sta interface in ap mode. Dropping packet...");
			return;
		}
		break;
	default:
		LOG_ERR("Network interface not connected. Current state: %s",
			wfx200_state_to_str(wfx200_0->state));
	}

	pkt = net_pkt_rx_alloc_with_buffer(wfx200_0->iface, rx_buffer->body.frame_length,
					   AF_UNSPEC, 0, K_NO_WAIT);
	if (!pkt) {
		LOG_ERR("Failed to get net buffer");
		return;
	}
	if (net_pkt_write(pkt, ((uint8_t *)rx_buffer->body.frame) + rx_buffer->body.frame_padding,
			  rx_buffer->body.frame_length) < 0) {
		LOG_ERR("Failed to write pkt");
		goto pkt_unref;
	}
	if ((res = net_recv_data(wfx200_0->iface, pkt)) < 0) {
		LOG_ERR("Failed to push received data %d", res);
		goto pkt_unref;
	}
	return;

pkt_unref:
	net_pkt_unref(pkt);
	return;
}

void sl_wfx_scan_result_callback(sl_wfx_scan_result_ind_t *scan_result)
{
	struct sl_wfx_scan_result_ind_body_s *body = &scan_result->body;
	int rssi = (body->rcpi / 2) - 110;
	struct wifi_scan_result wifi_scan_result = { 0 };

	if (wfx200_0->state < WFX200_STATE_INTERFACE_INITIALIZED ||
	    wfx200_0->scan_cb == NULL) {
		return;
	}
	memcpy(wifi_scan_result.ssid, body->ssid_def.ssid,
	       MIN(sizeof(wifi_scan_result.ssid), body->ssid_def.ssid_length));
	wifi_scan_result.channel = body->channel;
	wifi_scan_result.rssi = rssi;
	wifi_scan_result.security = body->security_mode.psk ?
				    WIFI_SECURITY_TYPE_PSK :
				    WIFI_SECURITY_TYPE_NONE;
	wfx200_0->scan_cb(wfx200_0->iface, 0, &wifi_scan_result);
}

void sl_wfx_scan_complete_callback(sl_wfx_scan_complete_ind_t *scan_complete)
{
	ARG_UNUSED(scan_complete);

	if (wfx200_0->state < WFX200_STATE_INTERFACE_INITIALIZED) {
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

void sl_wfx_ap_client_connected_callback(sl_wfx_ap_client_connected_ind_t *ap_client_connected)
{
	struct sl_wfx_ap_client_connected_ind_body_s *body = &ap_client_connected->body;

	LOG_INF("Client %02x:%02x:%02x:%02x:%02x:%02x connected to AP",
		body->mac[0],
		body->mac[1],
		body->mac[2],
		body->mac[3],
		body->mac[4],
		body->mac[5]);
}

void sl_wfx_ap_client_rejected_callback(sl_wfx_ap_client_rejected_ind_t *ap_client_rejected)
{
	ARG_UNUSED(ap_client_rejected);
	LOG_DBG("%s", __func__);
}

void sl_wfx_ap_client_disconnected_callback(sl_wfx_ap_client_disconnected_ind_t *ap_client_disconnected)
{
	struct sl_wfx_ap_client_disconnected_ind_body_s *body = &ap_client_disconnected->body;

	LOG_INF("Client %02x:%02x:%02x:%02x:%02x:%02x disconnected from AP",
		body->mac[0],
		body->mac[1],
		body->mac[2],
		body->mac[3],
		body->mac[4],
		body->mac[5]);
}

void sl_wfx_ext_auth_callback(sl_wfx_ext_auth_ind_t *ext_auth_indication)
{
	ARG_UNUSED(ext_auth_indication);
	LOG_DBG("%s", __func__);
}

sl_status_t sl_wfx_host_allocate_buffer(void **buffer,
					sl_wfx_buffer_type_t type,
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
						  uint8_t *header,
						  uint16_t header_length,
						  uint8_t *buffer,
						  uint16_t buffer_length)
{
	const struct wfx200_config *config = wfx200_0->dev->config;
	int err;
	const struct spi_buf_set tx = {
		.buffers = (const struct spi_buf[]){ {
							     .buf = header,
							     .len = header_length
						     }, {
							     .buf = buffer,
							     .len = buffer_length
						     }, },
		.count = (type == SL_WFX_BUS_WRITE)?2:1,
	};
	const struct spi_buf_set rx = {
		.buffers = (const struct spi_buf[]){ {
							     .buf = NULL,
							     .len = header_length
						     }, {
							     .buf = buffer,
							     .len = buffer_length
						     }, },
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
BUILD_ASSERT(CONFIG_WIFI_WFX200_LOG_BUFFER_SIZE > sizeof(TRUNCATED_MESSAGE),
	     "Log buffer must allow \"" TRUNCATED_MESSAGE "\" message");

void sl_wfx_host_log(const char *string, ...)
{
	if (IS_ENABLED(CONFIG_WIFI_WFX200_VERBOSE_DEBUG)) {
		va_list args;
		static char log_buffer[CONFIG_WIFI_WFX200_LOG_BUFFER_SIZE];
		int ret;

		va_start(args, string);
		ret = vsnprintf(log_buffer, sizeof(log_buffer), string, args);
		if (ret >= sizeof(log_buffer)) {
			memcpy(log_buffer + sizeof(log_buffer) -
			       sizeof(TRUNCATED_MESSAGE),
			       TRUNCATED_MESSAGE,
			       sizeof(TRUNCATED_MESSAGE));
		}
		log_buffer[sizeof(log_buffer) - 1] = 0;
		LOG_DBG("wfx-fullmac-driver: %s", log_buffer);
		va_end(args);
	}
}
