/*
 * SPDX-FileCopyrightText: Copyright 2025 - 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/sys/byteorder.h>
#include "usbh_class.h"
#include "usbh_device.h"
#include "usbh_desc.h"
#include "usbh_ch9.h"
#include "usbh_hub.h"
#include "usbh_hub_mgr.h"

LOG_MODULE_REGISTER(usbh_hub_mgr, CONFIG_USBH_HUB_LOG_LEVEL);

static struct {
	uint8_t total_hubs;
	sys_slist_t hub_list;
	struct k_mutex lock;
	struct usbh_context *uhs_ctx;
	struct usbh_hub_mgr_data *processing_hub;
} hub_mgr;

static int hub_mgr_interrupt_in_cb(struct usb_device *const dev,
				   struct uhc_transfer *const xfer);

/* Resubmit interrupt IN transfer */
static int hub_mgr_resubmit_interrupt_in(struct usbh_hub_mgr_data *hub_mgr_data,
					 struct uhc_transfer *xfer)
{
	struct net_buf *buf;
	int ret;

	if (!hub_mgr_data->connected || (hub_mgr_data->int_ep == NULL) ||
	    hub_mgr_data->being_removed) {
		usbh_xfer_free(hub_mgr_data->hub_instance.udev, xfer);
		return -ENODEV;
	}

	/* Allocate buffer for next transfer */
	buf = usbh_xfer_buf_alloc(hub_mgr_data->hub_instance.udev,
				  sys_le16_to_cpu(hub_mgr_data->int_ep->wMaxPacketSize));
	if (buf == NULL) {
		LOG_ERR("Failed to allocate interrupt IN buffer");
		usbh_xfer_free(hub_mgr_data->hub_instance.udev, xfer);
		return -ENOMEM;
	}

	xfer->buf = buf;

	ret = usbh_xfer_enqueue(hub_mgr_data->hub_instance.udev, xfer);
	if (ret != 0) {
		LOG_ERR("Failed to resubmit interrupt IN transfer: %d", ret);
		usbh_xfer_buf_free(hub_mgr_data->hub_instance.udev, buf);
		usbh_xfer_free(hub_mgr_data->hub_instance.udev, xfer);
		return ret;
	}

	hub_mgr_data->int_active = true;
	return 0;
}

/* Start hub interrupt monitoring */
static int hub_mgr_start_interrupt(struct usbh_hub_mgr_data *hub_mgr_data)
{
	struct uhc_transfer *xfer;
	struct net_buf *buf;
	int ret;

	if (!hub_mgr_data || hub_mgr_data->being_removed || hub_mgr_data->int_active) {
		return -EINVAL;
	}

	if (hub_mgr_data->state != HUB_STATE_OPERATIONAL) {
		return -ENOENT;
	}

	if (hub_mgr_data->int_ep == NULL) {
		LOG_ERR("No interrupt endpoint available");
		return -ENODEV;
	}

	xfer = usbh_xfer_alloc(hub_mgr_data->hub_instance.udev,
			       hub_mgr_data->int_ep->bEndpointAddress,
			       hub_mgr_interrupt_in_cb,
			       (void *)hub_mgr_data);
	if (xfer == NULL) {
		LOG_ERR("Failed to allocate interrupt transfer");
		return -ENOMEM;
	}

	hub_mgr_data->interrupt_transfer = xfer;

	buf = usbh_xfer_buf_alloc(hub_mgr_data->hub_instance.udev,
				  sys_le16_to_cpu(hub_mgr_data->int_ep->wMaxPacketSize));
	if (buf == NULL) {
		LOG_ERR("Failed to allocate interrupt buffer");
		usbh_xfer_free(hub_mgr_data->hub_instance.udev, xfer);
		return -ENOMEM;
	}

	xfer->buf = buf;

	ret = usbh_xfer_enqueue(hub_mgr_data->hub_instance.udev, xfer);
	if (ret != 0) {
		LOG_ERR("Failed to enqueue interrupt transfer: %d", ret);
		usbh_xfer_buf_free(hub_mgr_data->hub_instance.udev, buf);
		usbh_xfer_free(hub_mgr_data->hub_instance.udev, xfer);
		return ret;
	}

	hub_mgr_data->interrupt_transfer = xfer;
	hub_mgr_data->int_active = true;

	LOG_DBG("Hub level %d interrupt monitoring started",
		hub_mgr_data->hub_instance.udev->level);
	return 0;
}

/* Find hub manager data by USB device */
static struct usbh_hub_mgr_data *const find_hub_mgr_by_udev(struct usb_device *udev)
{
	struct usbh_hub_mgr_data *hub_mgr_data;

	k_mutex_lock(&hub_mgr.lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER(&hub_mgr.hub_list, hub_mgr_data, node) {
		if (hub_mgr_data->hub_instance.udev == udev) {
			k_mutex_unlock(&hub_mgr.lock);
			return hub_mgr_data;
		}
	}

	k_mutex_unlock(&hub_mgr.lock);

	return NULL;
}

/* Process hub interrupt data */
static void hub_mgr_process_data(struct usbh_hub_mgr_data *const hub_mgr_data)
{
	struct usb_hub_status *hub_sts;
	uint16_t hub_status;
	uint16_t hub_change;
	uint8_t port_index;
	int ret;

	k_mutex_lock(&hub_mgr_data->lock, K_FOREVER);

	if (hub_mgr_data->being_removed) {
		k_mutex_unlock(&hub_mgr_data->lock);
		return;
	}

	if (hub_mgr_data->state != HUB_STATE_OPERATIONAL) {
		LOG_DBG("Hub level %d not operational yet, deferring interrupt",
			hub_mgr_data->hub_instance.udev->level);

		k_mutex_unlock(&hub_mgr_data->lock);
		if (!hub_mgr_data->int_active && !hub_mgr_data->being_removed) {
			ret = hub_mgr_start_interrupt(hub_mgr_data);
			if (ret != 0) {
				LOG_ERR("Failed to start interrupt monitoring: %d", ret);
			}
		}

		return;
	}

	for (port_index = 0; port_index <= hub_mgr_data->hub_instance.ports; ++port_index) {
		if (((0x01U << (port_index & 0x07U)) &
		     (hub_mgr_data->int_buffer[port_index >> 3U])) == 0) {
			continue;
		}

		if (port_index != 0) {
			if (hub_mgr.processing_hub != NULL &&
			    hub_mgr.processing_hub != hub_mgr_data) {
				continue;
			}

			hub_mgr.processing_hub = hub_mgr_data;
			hub_mgr_data->current_port = port_index;

			LOG_DBG("Hub level %d port %d status changed, starting processing",
				hub_mgr_data->hub_instance.udev->level, port_index);

			k_mutex_unlock(&hub_mgr_data->lock);

			k_work_submit(&hub_mgr_data->port_work.work);
			return;
		}

		hub_sts = &hub_mgr_data->hub_instance.status;
		LOG_INF("Hub level %d status changed, processing",
			hub_mgr_data->hub_instance.udev->level);
		ret = usbh_req_get_hub_status(hub_mgr_data->hub_instance.udev,
					      &hub_status,
					      &hub_change);
		if (ret != 0) {
			LOG_ERR("Failed to get hub status: %d", ret);
			continue;
		}

		hub_sts->wHubStatus = hub_status;
		hub_sts->wHubChange = hub_change;

		LOG_DBG("Hub status: 0x%04x, change: 0x%04x", hub_sts->wHubStatus,
			hub_sts->wHubChange);

		if ((hub_sts->wHubChange & USB_HUB_CHANGE_LOCAL_POWER) != 0) {
			LOG_WRN("Hub local power status changed");
			ret = usbh_req_clear_hcfs_c_hub_local_power(
				hub_mgr_data->hub_instance.udev);
			if (ret != 0) {
				LOG_ERR("Failed to clear hub local power feature: %d", ret);
			}
		}

		if ((hub_sts->wHubChange & USB_HUB_CHANGE_OVER_CURRENT) != 0) {
			LOG_ERR("Hub over-current detected!");
			ret = usbh_req_clear_hcfs_c_hub_over_current(
				hub_mgr_data->hub_instance.udev);
			if (ret != 0) {
				LOG_ERR("Failed to clear hub over-current feature: %d",
					ret);
			}
		}
	}

	k_mutex_unlock(&hub_mgr_data->lock);

	if (!hub_mgr_data->int_active && !hub_mgr_data->being_removed) {
		ret = hub_mgr_start_interrupt(hub_mgr_data);
		if (ret != 0) {
			LOG_ERR("Failed to start interrupt monitoring: %d", ret);
		}
	}
}

/* Hub interrupt IN callback */
static int hub_mgr_interrupt_in_cb(struct usb_device *const dev,
				   struct uhc_transfer *const xfer)
{
	struct usbh_hub_mgr_data *const hub_mgr_data = (void *)xfer->priv;
	struct net_buf *buf = xfer->buf;
	int ret = 0;

	if (hub_mgr_data == NULL) {
		goto cleanup_and_exit;
	}

	k_mutex_lock(&hub_mgr_data->lock, K_FOREVER);

	if (hub_mgr_data->being_removed) {
		k_mutex_unlock(&hub_mgr_data->lock);
		goto cleanup_and_exit;
	}

	hub_mgr_data->int_active = false;

	if (buf == NULL || buf->len == 0) {
		LOG_ERR("Hub level %d interrupt transfer failed or no data",
			hub_mgr_data->hub_instance.udev->level);
		hub_mgr.processing_hub = NULL;
		hub_mgr_data->current_port = 0;
		k_mutex_unlock(&hub_mgr_data->lock);
		goto resubmit;
	}

	memcpy(hub_mgr_data->int_buffer, buf->data,
	       MIN(buf->len, sizeof(hub_mgr_data->int_buffer)));

	LOG_DBG("Hub level %d interrupt data received: length=%d",
		hub_mgr_data->hub_instance.udev->level,
		buf->len);

	k_mutex_unlock(&hub_mgr_data->lock);

	hub_mgr_process_data(hub_mgr_data);

	net_buf_unref(buf);
	return 0;

resubmit:
	/* Resubmit transfer */
	if (hub_mgr_data->connected && hub_mgr_data->state == HUB_STATE_OPERATIONAL) {
		ret = hub_mgr_resubmit_interrupt_in(hub_mgr_data, xfer);
		if (ret != 0) {
			LOG_ERR("Failed to resubmit interrupt transfer: %d", ret);
		}
	} else {
		usbh_xfer_free(hub_mgr_data->hub_instance.udev, xfer);
	}
	return 0;

cleanup_and_exit:
	if (buf != NULL) {
		net_buf_unref(buf);
	}
	usbh_xfer_free(dev, xfer);
	return 0;
}

/* Hub process state machine */
static void hub_mgr_process(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct usbh_hub_mgr_data *hub_mgr_data =
		CONTAINER_OF(dwork, struct usbh_hub_mgr_data, hub_work);
	struct usb_device *udev;
	uint16_t total_hub_desc_len = 0;
	bool prime_interrupt = false;
	bool process_success = false;
	int ret;

	k_mutex_lock(&hub_mgr_data->lock, K_FOREVER);

	if (hub_mgr_data->being_removed) {
		k_mutex_unlock(&hub_mgr_data->lock);
		return;
	}

	switch (hub_mgr_data->hub_status) {
	case HUB_RUN_IDLE:
		break;

	case HUB_RUN_INVALID:
		LOG_ERR("Hub in invalid state");
		hub_mgr_data->state = HUB_STATE_ERROR;
		break;

	case HUB_RUN_WAIT_SET_INTERFACE:
		hub_mgr_data->hub_status = HUB_RUN_GET_DESCRIPTOR_7;
		ret = usbh_req_desc_hub(hub_mgr_data->hub_instance.udev,
					hub_mgr_data->hub_instance.hub_desc_buf,
					sizeof(struct usb_hub_descriptor));
		if (ret == 0) {
			process_success = true;
			LOG_DBG("Getting 7-byte hub descriptor");
			k_work_submit(&hub_mgr_data->hub_work.work);
		} else {
			LOG_ERR("Failed to get hub descriptor: %d", ret);
			hub_mgr_data->hub_status = HUB_RUN_INVALID;
		}
		break;

	case HUB_RUN_GET_DESCRIPTOR_7: {
		udev = hub_mgr_data->hub_instance.udev;
		hub_mgr_data->hub_instance.ports = hub_mgr_data->hub_instance.hub_desc.bNbrPorts;

		LOG_DBG("Hub has %d ports", hub_mgr_data->hub_instance.ports);

		/* Store hub think time */
		udev->tt = USB_HUB_GET_THINK_TIME(
			sys_le16_to_cpu(hub_mgr_data->hub_instance.hub_desc.wHubCharacteristics));
		LOG_DBG("hub think time: 0x%04x", udev->tt);

		hub_mgr_data->hub_status = HUB_RUN_SET_PORT_POWER;

		total_hub_desc_len = 7 + ((hub_mgr_data->hub_instance.ports + 7) >> 3) + 1;
		/* Get full hub descriptor */
		ret = usbh_req_desc_hub(hub_mgr_data->hub_instance.udev,
					hub_mgr_data->hub_instance.hub_desc_buf,
					total_hub_desc_len);
		if (ret == 0) {
			process_success = true;
			LOG_DBG("Getting full hub descriptor");
			k_work_submit(&hub_mgr_data->hub_work.work);
		} else {
			LOG_ERR("Failed to get full hub descriptor: %d", ret);
			hub_mgr_data->hub_status = HUB_RUN_INVALID;
		}
		break;
	}

	case HUB_RUN_SET_PORT_POWER:
		/* Allocate port list if not already done */
		if (hub_mgr_data->port_list == NULL) {
			hub_mgr_data->port_list = k_malloc(hub_mgr_data->hub_instance.ports *
							   sizeof(struct usb_hub_port));
			if (hub_mgr_data->port_list == NULL) {
				LOG_ERR("Failed to allocate port list");
				hub_mgr_data->hub_status = HUB_RUN_INVALID;
				break;
			}
			hub_mgr_data->port_index = 0;
		}

		if (hub_mgr_data->port_index < hub_mgr_data->hub_instance.ports) {
			hub_mgr_data->port_index++;

			ret = usbh_req_set_hcfs_ppwr(hub_mgr_data->hub_instance.udev,
						     hub_mgr_data->port_index);
			if (ret == 0) {
				process_success = true;
				LOG_DBG("Setting port %d power", hub_mgr_data->port_index);
				k_work_submit(&hub_mgr_data->hub_work.work);
				break;
			}

			LOG_ERR("Failed to set port %d power: %d", hub_mgr_data->port_index, ret);
			prime_interrupt = true;
			break;
		}

		for (uint8_t i = 0; i < hub_mgr_data->hub_instance.ports; i++) {
			hub_mgr_data->port_list[i].udev = NULL;
			hub_mgr_data->port_list[i].reset_count = CONFIG_USBH_HUB_PORT_RESET_TIMES;
			hub_mgr_data->port_list[i].status = HUB_PORT_RUN_WAIT_PORT_CHANGE;
			hub_mgr_data->port_list[i].state = PORT_STATE_DISCONNECTED;
			hub_mgr_data->port_list[i].num = i + 1;
			hub_mgr_data->port_list[i].speed = USB_SPEED_SPEED_FS;
		}

		hub_mgr_data->hub_status = HUB_RUN_IDLE;
		hub_mgr_data->state = HUB_STATE_OPERATIONAL;
		prime_interrupt = true;
		break;

	default:
		LOG_ERR("Unknown hub status: %d", hub_mgr_data->hub_status);
		hub_mgr_data->hub_status = HUB_RUN_INVALID;
		hub_mgr_data->state = HUB_STATE_ERROR;
		break;
	}

	k_mutex_unlock(&hub_mgr_data->lock);

	if (prime_interrupt) {
		hub_mgr_data->hub_status = HUB_RUN_IDLE;
		if (!hub_mgr_data->int_active && !hub_mgr_data->being_removed) {
			ret = hub_mgr_start_interrupt(hub_mgr_data);
			if (ret != 0) {
				LOG_ERR("Failed to start interrupt monitoring: %d", ret);
			}
		}
	} else if (!process_success && hub_mgr_data->hub_status != HUB_RUN_INVALID) {
		hub_mgr_data->hub_status = HUB_RUN_INVALID;
		hub_mgr_data->state = HUB_STATE_ERROR;
	}
}

/* Recursively disconnect hub and all children */
static void hub_mgr_recursive_disconnect(struct usbh_hub_mgr_data *const hub_mgr_data)
{
	struct usbh_hub_mgr_data *child_hub;
	struct usb_device *port_udev;

	LOG_DBG("Recursively disconnecting Hub level %d and all children",
		hub_mgr_data->hub_instance.udev->level);

	k_work_cancel_delayable(&hub_mgr_data->port_work);
	k_work_cancel_delayable(&hub_mgr_data->hub_work);

	k_mutex_lock(&hub_mgr_data->lock, K_FOREVER);

	hub_mgr_data->int_active = false;

	if (hub_mgr.processing_hub == hub_mgr_data) {
		hub_mgr.processing_hub = NULL;
		hub_mgr_data->current_port = 0;
	}

	k_mutex_unlock(&hub_mgr_data->lock);

	for (uint8_t i = 0; i < hub_mgr_data->hub_instance.ports; i++) {
		if (hub_mgr_data->port_list && hub_mgr_data->port_list[i].udev) {

			port_udev = hub_mgr_data->port_list[i].udev;
			child_hub = find_hub_mgr_by_udev(port_udev);
			if (child_hub != NULL) {
				LOG_DBG("Found child Hub on port %d, recursing", i + 1);
				hub_mgr_recursive_disconnect(child_hub);

			} else {
				LOG_DBG("Disconnecting device on port %d", i + 1);
				usbh_device_disconnect(hub_mgr_data->uhs_ctx, port_udev);
			}

			/* Clear port state */
			hub_mgr_data->port_list[i].udev = NULL;
			hub_mgr_data->port_list[i].state = PORT_STATE_DISCONNECTED;
		}
	}

	if (!hub_mgr_data->being_removed) {
		LOG_DBG("Triggering Hub level %d removal", hub_mgr_data->hub_instance.udev->level);
		usbh_device_disconnect(hub_mgr_data->uhs_ctx, hub_mgr_data->hub_instance.udev);
	}
}

/* Print hub information */
static void hub_print_info(struct usbh_hub_mgr_data *const hub_mgr_data)
{
	const struct usb_device_descriptor *const dev_desc =
					&hub_mgr_data->hub_instance.udev->dev_desc;
	struct usb_device *udev = hub_mgr_data->hub_instance.udev;

	LOG_INF("=== USB Hub Information ===");
	LOG_INF("Hub Level: %d", udev->level);
	LOG_INF("Vendor ID: 0x%04x", sys_le16_to_cpu(dev_desc->idVendor));
	LOG_INF("Product ID: 0x%04x", sys_le16_to_cpu(dev_desc->idProduct));
	LOG_INF("Device Address: %d", udev->addr);
	if (udev->hub) {
		LOG_INF("Parent Hub Level: %d, Port: %d", udev->hub->level,
			udev->hub_port);
	} else {
		LOG_INF("Root Hub (no parent)");
	}
	LOG_INF("===========================");
}

/* Establish parent-child hub relationship */
static int hub_establish_parent_child_relationship(struct usbh_hub_mgr_data *const parent_hub,
						   struct usbh_hub_mgr_data *const child_hub,
						   uint8_t port_num)
{
	if (child_hub->hub_instance.udev->level > CONFIG_USBH_HUB_MAX_LEVELS) {
		LOG_ERR("Hub chain depth limit exceeded (%d > %d), removing hub",
			child_hub->hub_instance.udev->level,
			CONFIG_USBH_HUB_MAX_LEVELS);

		k_mutex_lock(&child_hub->lock, K_FOREVER);
		child_hub->being_removed = true;
		child_hub->state = HUB_STATE_ERROR;
		k_mutex_unlock(&child_hub->lock);

		hub_mgr_recursive_disconnect(child_hub);

		return -ENOSPC;
	}

	k_mutex_lock(&child_hub->lock, K_FOREVER);
	child_hub->hub_instance.udev->hub = parent_hub->hub_instance.udev;
	child_hub->hub_instance.udev->hub_port = port_num;
	sys_slist_append(&parent_hub->child_hubs, &child_hub->child_node);
	k_mutex_unlock(&child_hub->lock);

	hub_print_info(child_hub);

	return 0;
}

/* Port processing state machine */
static void hub_port_process(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct usbh_hub_mgr_data *const hub_mgr_data =
		CONTAINER_OF(dwork, struct usbh_hub_mgr_data, port_work);
	struct usb_hub_port *port_instance = NULL;
	struct usb_hub_port_status *port_sts;
	struct usbh_hub_mgr_data *child_hub;
	struct usb_device *hub_udev;
	struct usb_device *udev;
	bool restart_interrupt = false;
	bool process_complete = false;
	uint16_t port_status;
	uint16_t port_change;
	uint8_t feature = 0;
	uint8_t port_num;
	int ret;

	port_num = hub_mgr_data->current_port;
	if (port_num == 0 || port_num > hub_mgr_data->hub_instance.ports ||
	    hub_mgr_data->port_list == NULL) {
		LOG_ERR("Invalid port state for processing");
		goto exit_processing;
	}

	port_instance = &hub_mgr_data->port_list[port_num - 1];

	k_mutex_lock(&hub_mgr_data->lock, K_FOREVER);

	if (hub_mgr_data->being_removed) {
		k_mutex_unlock(&hub_mgr_data->lock);
		return;
	}

	if (hub_mgr_data->hub_instance.udev == NULL ||
	    hub_mgr_data->hub_instance.udev->state != USB_STATE_CONFIGURED) {
		LOG_ERR("Hub device not ready");
		k_work_reschedule(&hub_mgr_data->port_work, K_MSEC(CONFIG_USBH_HUB_CONFIG_WAIT_MS));
		k_mutex_unlock(&hub_mgr_data->lock);
		return;
	}

	k_mutex_unlock(&hub_mgr_data->lock);

	LOG_DBG("Processing port %d, status=%d", port_num, port_instance->status);

	/* Port state machine main logic */
	switch (port_instance->status) {
	case HUB_PORT_RUN_WAIT_PORT_CHANGE: {
		port_sts = &hub_mgr_data->hub_instance.port_status;

		LOG_DBG("Port %d: Getting port status", port_num);

		port_instance->status = HUB_PORT_RUN_CHECK_C_PORT_CONNECTION;
		ret = usbh_req_get_port_status(hub_mgr_data->hub_instance.udev, port_num,
					       &port_status, &port_change);
		if (ret != 0) {
			LOG_ERR("Failed to get port status: %d", ret);
			goto error_recovery;
		}

		port_sts->wPortStatus = port_status;
		port_sts->wPortChange = port_change;

		k_work_submit(&hub_mgr_data->port_work.work);
		return;
	}

	case HUB_PORT_RUN_CHECK_C_PORT_CONNECTION: {
		port_sts = &hub_mgr_data->hub_instance.port_status;
		LOG_DBG("Port %d status: wPortStatus=0x%04x, wPortChange=0x%04x", port_num,
			port_sts->wPortStatus, port_sts->wPortChange);

		if ((port_sts->wPortChange & USB_HUB_PORT_CHANGE_CONNECTION) != 0) {
			/* Connection status change */
			port_instance->status = HUB_PORT_RUN_GET_PORT_CONNECTION;
			ret = usbh_req_clear_hcfs_c_pconnection(
							hub_mgr_data->hub_instance.udev, port_num);
			if (ret != 0) {
				LOG_ERR("Failed to clear port connection change: %d", ret);
				goto error_recovery;
			}

			LOG_DBG("Port %d: Cleared connection change bit", port_num);
			k_work_submit(&hub_mgr_data->port_work.work);
			return;

		} else if ((port_sts->wPortStatus & USB_HUB_PORT_STATUS_CONNECTION) != 0) {
			/* Device connected, need reset */
			LOG_INF("Device connected to port %d, starting reset", port_num);
			port_instance->status = HUB_PORT_RUN_WAIT_PORT_RESET_DONE;
			ret = usbh_req_set_hcfs_prst(hub_mgr_data->hub_instance.udev, port_num);
			if (ret != 0) {
				LOG_ERR("Failed to reset port: %d", ret);
				goto error_recovery;
			}

			k_work_submit(&hub_mgr_data->hub_work.work);
			return;
		}

		if ((port_sts->wPortChange & USB_HUB_PORT_CHANGE_RESET) != 0) {
			/* Reset completed */
			feature = USB_HCFS_C_PORT_RESET;
			port_instance->status = HUB_PORT_RUN_CHECK_C_PORT_RESET;

		} else if ((port_sts->wPortChange & USB_HUB_PORT_CHANGE_ENABLE) != 0) {
			/* Enable status change */
			feature = USB_HCFS_C_PORT_ENABLE;
			port_instance->status = HUB_PORT_RUN_WAIT_PORT_CHANGE;

		} else if ((port_sts->wPortChange & USB_HUB_PORT_CHANGE_OVER_CURRENT) != 0) {
			/* Over-current detection */
			feature = USB_HCFS_C_PORT_OVER_CURRENT;
			port_instance->status = HUB_PORT_RUN_WAIT_PORT_CHANGE;
			LOG_WRN("Port %d over-current detected", port_num);

		} else if ((port_sts->wPortStatus & USB_HUB_PORT_STATUS_CONNECTION) == 0) {
			/* Device disconnected */
			goto process_disconnection;
		}

		if (feature != 0) {
			if (feature == USB_HCFS_C_PORT_RESET) {
				ret = usbh_req_clear_hcfs_c_preset(
					hub_mgr_data->hub_instance.udev, port_num);
			} else if (feature == USB_HCFS_C_PORT_ENABLE) {
				ret = usbh_req_clear_hcfs_c_penable(
					hub_mgr_data->hub_instance.udev, port_num);
			} else if (feature == USB_HCFS_C_PORT_OVER_CURRENT) {
				ret = usbh_req_clear_hcfs_c_pover_current(
					hub_mgr_data->hub_instance.udev, port_num);
			}
			if (ret != 0) {
				LOG_ERR("Failed to clear feature %d: %d", feature, ret);
				goto error_recovery;
			}
			LOG_DBG("Port %d: Cleared feature %d", port_num, feature);
			k_work_submit(&hub_mgr_data->port_work.work);
			return;
		}
		break;
	}

	case HUB_PORT_RUN_GET_PORT_CONNECTION: {
		port_sts = &hub_mgr_data->hub_instance.port_status;
		port_instance->status = HUB_PORT_RUN_CHECK_PORT_CONNECTION;
		ret = usbh_req_get_port_status(hub_mgr_data->hub_instance.udev, port_num,
					       &port_status, &port_change);
		if (ret != 0) {
			LOG_ERR("Failed to get port connection status: %d", ret);
			goto error_recovery;
		}

		port_sts->wPortStatus = port_status;
		port_sts->wPortChange = port_change;

		k_work_submit(&hub_mgr_data->port_work.work);
		return;
	}
	case HUB_PORT_RUN_CHECK_PORT_CONNECTION: {
		port_sts = &hub_mgr_data->hub_instance.port_status;
		if ((port_sts->wPortStatus & USB_HUB_PORT_STATUS_CONNECTION) == 0) {
			/* Device disconnected */
			goto process_disconnection;
		}

		/* Confirm connection, start reset */
		LOG_DBG("Port %d connection confirmed, resetting", port_num);
		port_instance->status = HUB_PORT_RUN_WAIT_PORT_RESET_DONE;
		ret = usbh_req_set_hcfs_prst(hub_mgr_data->hub_instance.udev, port_num);
		if (ret != 0) {
			LOG_ERR("Failed to reset port: %d", ret);
			goto error_recovery;
		}

		if (port_instance->reset_count > 0) {
			port_instance->reset_count--;
		}

		k_work_submit(&hub_mgr_data->port_work.work);
		return;
	}

	case HUB_PORT_RUN_WAIT_PORT_RESET_DONE:
		port_instance->status = HUB_PORT_RUN_WAIT_C_PORT_RESET;
		/* Wait for interrupt notification of reset completion */
		/* restart interrupt listening */
		if (!hub_mgr_data->int_active && !hub_mgr_data->being_removed) {
			ret = hub_mgr_start_interrupt(hub_mgr_data);
			if (ret != 0) {
				LOG_ERR("Failed to restart interrupt for reset wait: %d", ret);
			}
		}
		LOG_DBG("Port %d waiting for reset completion interrupt", port_num);
		return;

	case HUB_PORT_RUN_WAIT_C_PORT_RESET: {
		port_sts = &hub_mgr_data->hub_instance.port_status;
		port_instance->status = HUB_PORT_RUN_CHECK_C_PORT_RESET;
		ret = usbh_req_get_port_status(hub_mgr_data->hub_instance.udev, port_num,
			&port_status,
					       &port_change);
		if (ret != 0) {
			LOG_ERR("Failed to get port status for reset check: %d", ret);
			goto error_recovery;
		}

		port_sts->wPortStatus = port_status;
		port_sts->wPortChange = port_change;

		k_work_submit(&hub_mgr_data->port_work.work);
		return;
	}

	case HUB_PORT_RUN_CHECK_C_PORT_RESET: {
		port_sts = &hub_mgr_data->hub_instance.port_status;
		if ((port_sts->wPortChange & USB_HUB_PORT_CHANGE_RESET) == 0) {
			/* Reset not completed */
			LOG_DBG("Port %d reset not completed, checking again", port_num);
			port_instance->status = HUB_PORT_RUN_WAIT_C_PORT_RESET;
			k_work_reschedule(&hub_mgr_data->port_work, K_MSEC(100));
			return;
		}

		if (port_instance->reset_count == 0) {
			port_instance->status = HUB_PORT_RUN_PORT_ATTACHED;
			if ((port_sts->wPortStatus & USB_HUB_PORT_STATUS_HIGH_SPEED) != 0) {
				port_instance->speed = USB_SPEED_SPEED_HS;
			} else if ((port_sts->wPortStatus & USB_HUB_PORT_STATUS_LOW_SPEED) != 0) {
				port_instance->speed = USB_SPEED_SPEED_LS;
			} else {
				port_instance->speed = USB_SPEED_SPEED_FS;
			}

			LOG_INF("Device ready on port %d (speed: %s)", port_num,
				port_instance->speed == USB_SPEED_SPEED_HS ? "HIGH" :
				port_instance->speed == USB_SPEED_SPEED_LS ? "LOW" : "FULL");
		} else {
			/* Need another reset */
			port_instance->status = HUB_PORT_RUN_RESET_AGAIN;
		}

		ret = usbh_req_clear_hcfs_c_preset(hub_mgr_data->hub_instance.udev,
						   port_num);
		if (ret != 0) {
			LOG_ERR("Failed to clear port reset feature: %d", ret);
			goto error_recovery;
		}

		k_work_submit(&hub_mgr_data->port_work.work);
		return;
	}

	case HUB_PORT_RUN_RESET_AGAIN: {
		port_sts = &hub_mgr_data->hub_instance.port_status;

		LOG_DBG("Port %d retrying reset (%d attempts left)", port_num,
			port_instance->reset_count);
		port_instance->status = HUB_PORT_RUN_CHECK_PORT_CONNECTION;
		ret = usbh_req_get_port_status(hub_mgr_data->hub_instance.udev, port_num,
					       &port_status,
					       &port_change);
		if (ret != 0) {
			LOG_ERR("Failed to get port status for reset again: %d", ret);
			goto error_recovery;
		}

		port_sts->wPortStatus = port_status;
		port_sts->wPortChange = port_change;

		k_work_submit(&hub_mgr_data->port_work.work);
		return;
	}

	case HUB_PORT_RUN_PORT_ATTACHED:
		LOG_INF("Device attached to port %d", port_num);

		/* Notify USB host stack to start device enumeration */
		udev = usbh_device_alloc(hub_mgr_data->uhs_ctx);

		if (udev != NULL) {
			udev->hub_port = port_num;
			udev->speed = port_instance->speed;
			udev->level = hub_mgr_data->hub_instance.udev->level + 1;
			udev->hub = hub_mgr_data->hub_instance.udev;

			usbh_device_connect(hub_mgr_data->uhs_ctx, udev);

			LOG_INF("Device enumeration completed for port %d, addr=%d", port_num,
				udev->addr);

			port_instance->udev = udev;

			child_hub = find_hub_mgr_by_udev(port_instance->udev);
			if (child_hub != NULL) {
				port_instance->status = HUB_PORT_RUN_CHECK_CHILD_HUB;
				/* for hub, delay to wait hub probe finished */
				k_work_reschedule(&hub_mgr_data->port_work,
						  K_MSEC(CONFIG_USBH_HUB_CHILD_PROBE_DELAY_MS));
				return;
			}
		} else {
			LOG_ERR("Device enumeration failed for port %d", port_num);

			/* Retry enumeration */
			if (port_instance->reset_count > 0) {
				port_instance->reset_count--;
				port_instance->status = HUB_PORT_RUN_WAIT_PORT_CHANGE;
				LOG_WRN("Port %d enumeration failed, %d retries left", port_num,
					port_instance->reset_count);
				k_work_reschedule(&hub_mgr_data->port_work,
						  K_MSEC(CONFIG_USBH_HUB_ENUM_RETRY_DELAY_MS));
				return;
			}

			LOG_ERR("Port %d enumeration max retries exceeded", port_num);
		}

		process_complete = true;
		hub_mgr_data->current_port = 0;
		hub_mgr.processing_hub = NULL;
		port_instance->reset_count = CONFIG_USBH_HUB_PORT_RESET_TIMES;
		restart_interrupt = true;

		goto exit_processing;

	case HUB_PORT_RUN_CHECK_CHILD_HUB: {
		hub_udev = hub_mgr_data->hub_instance.udev;

		hub_establish_parent_child_relationship(
			hub_mgr_data, find_hub_mgr_by_udev(port_instance->udev), port_num);

		/* Complete port processing */
		process_complete = true;
		hub_mgr_data->current_port = 0;
		hub_mgr.processing_hub = NULL;
		port_instance->reset_count = CONFIG_USBH_HUB_PORT_RESET_TIMES;
		restart_interrupt = true;

		goto exit_processing;
	}

	default:
		LOG_ERR("Unknown port status: %d", port_instance->status);
		goto error_recovery;
	}

	/* Process device disconnection */
process_disconnection:
	if (port_instance->udev != NULL) {
		LOG_INF("Device disconnected from Hub level %d port %d",
			hub_mgr_data->hub_instance.udev->level,
			port_num);

		udev = port_instance->udev;
		port_instance->udev = NULL;
		port_instance->state = PORT_STATE_DISCONNECTED;
		if (udev != NULL) {
			child_hub = find_hub_mgr_by_udev(udev);
			if (child_hub != NULL) {
				LOG_INF("Child Hub disconnected, triggering recursive removal");
				hub_mgr_recursive_disconnect(child_hub);
			} else {
				/* Normal device disconnect */
				usbh_device_disconnect(hub_mgr_data->uhs_ctx, udev);
			}
		}
	}
	process_complete = true;

exit_processing:
	/* Processing completed, cleanup state */
	if (process_complete ||
	    (port_instance != NULL && port_instance->status == HUB_PORT_RUN_INVALID)) {
		hub_mgr_data->current_port = 0;
		hub_mgr.processing_hub = NULL;
		port_instance->status = HUB_PORT_RUN_WAIT_PORT_CHANGE;
		port_instance->reset_count = CONFIG_USBH_HUB_PORT_RESET_TIMES;
		restart_interrupt = true;

		LOG_DBG("Port %d processing completed", port_num);
	}

	if (restart_interrupt && !hub_mgr_data->int_active && !hub_mgr_data->being_removed) {
		ret = hub_mgr_start_interrupt(hub_mgr_data);
		if (ret) {
			LOG_ERR("Failed to restart interrupt monitoring: %d", ret);
		}
	}

	return;

error_recovery:
	if (port_instance == NULL) {
		LOG_ERR("Port instance is NULL in error recovery");
		goto exit_processing;
	}

	if (port_instance->reset_count > 0) {
		port_instance->reset_count--;
		port_instance->status = HUB_PORT_RUN_WAIT_PORT_CHANGE;

		LOG_WRN("Port %d error recovery, %d retries left", port_num,
			port_instance->reset_count);
		k_work_reschedule(&hub_mgr_data->port_work,
				  K_MSEC(CONFIG_USBH_HUB_ERROR_RECOVERY_DELAY_MS));
	} else {
		LOG_ERR("Port %d max retries exceeded, disabling", port_num);
		port_instance->status = HUB_PORT_RUN_INVALID;
		goto exit_processing;
	}
}

/* USBH class implementation for HUB devices */
static int usbh_hub_mgr_probe(struct usbh_class_data *const c_data,
			      struct usb_device *const udev,
			      const uint8_t iface)
{
	struct usbh_hub_mgr_data *hub_mgr_data;
	const struct usb_desc_header *header;
	const void *desc_start;
	const void *desc_end;
	uint8_t target_iface;

	if (hub_mgr.uhs_ctx == NULL && udev != NULL && udev->ctx != NULL) {
		hub_mgr.uhs_ctx = (struct usbh_context *)udev->ctx;
	}

	if (hub_mgr.total_hubs == CONFIG_USBH_HUB_INSTANCES_COUNT) {
		LOG_ERR("Maximum number of hubs reached (%d)", CONFIG_USBH_HUB_INSTANCES_COUNT);
		return -ENOTSUP;
	}

	/* Convert device-level match to interface 0 */
	if (iface == USBH_CLASS_IFNUM_DEVICE) {
		target_iface = 0;
	} else {
		target_iface = iface;
	}

	LOG_DBG("USB HUB device probe at interface %u", target_iface);

	desc_start = usbh_desc_get_iface(udev, target_iface);
	if (desc_start == NULL) {
		LOG_ERR("Failed to find interface %u descriptor", iface);
		return -ENOTSUP;
	}

	/* Get the start of next function as the end of current function */
	desc_end = usbh_desc_get_next_function(desc_start);

	hub_mgr_data = k_malloc(sizeof(*hub_mgr_data));
	if (!hub_mgr_data) {
		LOG_ERR("Failed to allocate HUB management data");
		return -ENOTSUP;
	}

	memset(hub_mgr_data, 0, sizeof(*hub_mgr_data));

	usbh_hub_init_instance(&hub_mgr_data->hub_instance, udev);

	hub_mgr_data->hub_instance.udev = udev;
	hub_mgr_data->uhs_ctx = hub_mgr.uhs_ctx;
	hub_mgr_data->state = HUB_STATE_INIT;
	hub_mgr_data->hub_status = HUB_RUN_WAIT_SET_INTERFACE;
	hub_mgr_data->port_index = 0;
	hub_mgr_data->port_list = NULL;
	hub_mgr_data->being_removed = false;
	hub_mgr_data->interrupt_transfer = NULL;
	hub_mgr_data->int_active = false;

	sys_slist_init(&hub_mgr_data->child_hubs);

	/* Parse interrupt endpoint within the interface descriptors */
	header = (const void *)desc_start;
	while (header != NULL) {
		/* Stop if we've reached the next function */
		if ((desc_end != NULL) && ((void *)header >= desc_end)) {
			break;
		}

		if (header->bDescriptorType == USB_DESC_ENDPOINT) {
			const struct usb_ep_descriptor *ep_desc = (const void *)header;

			if ((ep_desc->bEndpointAddress & USB_EP_DIR_MASK) == USB_EP_DIR_IN &&
			    (ep_desc->bmAttributes & USB_EP_TRANSFER_TYPE_MASK) ==
				    USB_EP_TYPE_INTERRUPT) {
				hub_mgr_data->int_ep = ep_desc;
				LOG_DBG("Found hub interrupt IN endpoint 0x%02x",
					ep_desc->bEndpointAddress);
				break;
			}
		}

		header = usbh_desc_get_next(header);
	}

	hub_mgr_data->connected = true;
	hub_mgr_data->int_active = false;

	k_mutex_init(&hub_mgr_data->lock);
	k_work_init_delayable(&hub_mgr_data->hub_work, hub_mgr_process);
	k_work_init_delayable(&hub_mgr_data->port_work, hub_port_process);

	/* Here only print hub information for the first hub. */
	if (0U == hub_mgr.total_hubs) {
		hub_print_info(hub_mgr_data);
	}

	c_data->priv = hub_mgr_data;

	k_mutex_lock(&hub_mgr.lock, K_FOREVER);
	sys_slist_append(&hub_mgr.hub_list, &hub_mgr_data->node);
	hub_mgr.total_hubs++;
	k_mutex_unlock(&hub_mgr.lock);

	k_work_submit(&hub_mgr_data->hub_work.work);

	return 0;
}

/* USBH class implementation for HUB device removal */
static int usbh_hub_mgr_removed(struct usbh_class_data *const cdata)
{
	struct usbh_hub_mgr_data *hub_mgr_data;
	int ret;

	hub_mgr_data = cdata->priv;
	if (hub_mgr_data == hub_mgr.processing_hub) {
		hub_mgr.processing_hub = NULL;
	}

	k_mutex_lock(&hub_mgr_data->lock, K_FOREVER);
	hub_mgr_data->being_removed = true;
	k_mutex_unlock(&hub_mgr_data->lock);

	/* Recursively disconnect all child hubs and devices */
	hub_mgr_recursive_disconnect(hub_mgr_data);

	k_work_cancel_delayable(&hub_mgr_data->hub_work);
	k_work_cancel_delayable(&hub_mgr_data->port_work);

	k_mutex_lock(&hub_mgr_data->lock, K_FOREVER);

	if (hub_mgr_data->interrupt_transfer != NULL && hub_mgr_data->int_active) {
		ret = usbh_xfer_dequeue(hub_mgr_data->hub_instance.udev,
					hub_mgr_data->interrupt_transfer);
		if (ret != 0) {
			LOG_ERR("Failed to dequeue interrupt transfer: %d", ret);
		}

		if (hub_mgr_data->interrupt_transfer->buf != NULL) {
			usbh_xfer_buf_free(hub_mgr_data->hub_instance.udev,
					   hub_mgr_data->interrupt_transfer->buf);
		}
		usbh_xfer_free(hub_mgr_data->hub_instance.udev, hub_mgr_data->interrupt_transfer);

		hub_mgr_data->interrupt_transfer = NULL;
		hub_mgr_data->int_active = false;

		LOG_DBG("Interrupt transfer cancelled");
	}

	/* Remove all connected devices */
	for (uint8_t i = 0; i < hub_mgr_data->hub_instance.ports; i++) {
		if (hub_mgr_data->port_list != NULL &&
		    hub_mgr_data->port_list[i].udev != NULL) {
			hub_mgr_data->port_list[i].udev = NULL;
			hub_mgr_data->port_list[i].state = PORT_STATE_DISCONNECTED;
		}
	}

	if (hub_mgr_data->hub_instance.udev->hub != NULL) {
		struct usbh_hub_mgr_data *parent_hub_mgr;

		parent_hub_mgr = find_hub_mgr_by_udev(hub_mgr_data->hub_instance.udev->hub);
		if (parent_hub_mgr != NULL) {
			sys_slist_find_and_remove(&parent_hub_mgr->child_hubs,
						  &hub_mgr_data->child_node);
		}
	}

	sys_slist_find_and_remove(&hub_mgr.hub_list, &hub_mgr_data->node);
	if (hub_mgr.total_hubs > 0) {
		hub_mgr.total_hubs--;
	}

	k_mutex_unlock(&hub_mgr.lock);

	usbh_hub_cleanup_instance(&hub_mgr_data->hub_instance);

	/* Free port list */
	if (hub_mgr_data->port_list) {
		k_free(hub_mgr_data->port_list);
		hub_mgr_data->port_list = NULL;
	}

	LOG_INF("Hub (level %d, Vendor ID: 0x%04x, Product ID: 0x%04x) removal completed",
		hub_mgr_data->hub_instance.udev->level,
		sys_le16_to_cpu(hub_mgr_data->hub_instance.udev->dev_desc.idVendor),
		sys_le16_to_cpu(hub_mgr_data->hub_instance.udev->dev_desc.idProduct));

	k_free(hub_mgr_data);

	return 0;
}

/* Hub class initialization function */
static int usbh_hub_mgr_init(struct usbh_class_data *const c_data)
{
	return 0;
}

static struct usbh_class_filter hub_filters[] = {
	{
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
		.class = USB_HUB_CLASS_CODE,
		.sub = USB_HUB_SUBCLASS_CODE,
		.proto = 1,
	},
	{0},
};

/* Hub class API structure */
static struct usbh_class_api usbh_hub_class_api = {
	.init = usbh_hub_mgr_init,
	.probe = usbh_hub_mgr_probe,
	.removed = usbh_hub_mgr_removed,
};

static int hub_init(void)
{
	sys_slist_init(&hub_mgr.hub_list);
	k_mutex_init(&hub_mgr.lock);
	hub_mgr.total_hubs = 0;
	hub_mgr.processing_hub = NULL;
	hub_mgr.uhs_ctx = NULL;

	return 0;
}

SYS_INIT(hub_init, POST_KERNEL, CONFIG_USBH_INIT_PRIO);

#define USBH_DEFINE_HUB_CLASS(i, _)                                                                \
	USBH_DEFINE_CLASS(UTIL_CAT(usbh_hub_class_, i), &usbh_hub_class_api, NULL, hub_filters)

LISTIFY(CONFIG_USBH_HUB_INSTANCES_COUNT, USBH_DEFINE_HUB_CLASS, (;), _)
