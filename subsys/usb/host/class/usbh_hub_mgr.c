/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/sys/byteorder.h>
#include "usbh_class.h"
#include "usbh_device.h"
#include "usbh_desc.h"
#include "usbh_ch9.h"
#include "usbh_hub.h"
#include "usbh_hub_mgr.h"

LOG_MODULE_REGISTER(usbh_hub_mgr, LOG_LEVEL_INF);

static struct {
	sys_slist_t hub_list;
	struct k_mutex lock;
	struct usbh_context *uhs_ctx;
	uint16_t total_hubs;
	uint16_t max_concurrent_hubs;
	struct usbh_hub_mgr_data *current_hub_process; /* Current processing hub */
} hub_mgr;

/* Forward declarations */
static void usbh_hub_mgr_process(struct k_work *work);
static void usbh_hub_port_process(struct k_work *work);
static int usbh_hub_mgr_start_interrupt(struct usbh_hub_mgr_data *hub_mgr_data);
static int usbh_hub_mgr_interrupt_in_cb(struct usb_device *const dev,
					 struct uhc_transfer *const xfer);

/**
 * @brief Resubmit interrupt IN transfer
 */
static int usbh_hub_mgr_resubmit_interrupt_in(struct usbh_hub_mgr_data *hub_mgr_data,
					       struct uhc_transfer *xfer)
{
	struct net_buf *buf;
	int ret;

	if (!hub_mgr_data->connected || !hub_mgr_data->int_ep || hub_mgr_data->being_removed) {
		usbh_xfer_free(hub_mgr_data->hub_udev, xfer);
		return -ENODEV;
	}

	/* Allocate buffer for next transfer */
	buf = usbh_xfer_buf_alloc(hub_mgr_data->hub_udev, 
				  sys_le16_to_cpu(hub_mgr_data->int_ep->wMaxPacketSize));
	if (!buf) {
		LOG_ERR("Failed to allocate interrupt IN buffer");
		usbh_xfer_free(hub_mgr_data->hub_udev, xfer);
		return -ENOMEM;
	}

	/* Reuse transfer with new buffer */
	xfer->buf = buf;

	ret = usbh_xfer_enqueue(hub_mgr_data->hub_udev, xfer);
	if (ret != 0) {
		LOG_ERR("Failed to resubmit interrupt IN transfer: %d", ret);
		usbh_xfer_buf_free(hub_mgr_data->hub_udev, buf);
		usbh_xfer_free(hub_mgr_data->hub_udev, xfer);
		return ret;
	}

	hub_mgr_data->int_active = true;
	return 0;
}

/**
 * @brief Hub control transfer callback
 */
static void usbh_hub_control_callback(void *param, uint8_t *data, uint32_t data_len, int status)
{
	struct usbh_hub_mgr_data *hub_mgr_data = (struct usbh_hub_mgr_data *)param;

	if (!hub_mgr_data || hub_mgr_data->being_removed) {
		return;
	}

	/* Clear transfer state */
	hub_mgr_data->control_transfer = NULL;
	hub_mgr_data->prime_status = kPrimeNone;

	if (status != 0) {
		LOG_ERR("Hub level %d control transfer failed: %d", 
			hub_mgr_data->hub_level, status);
	}

	/* Continue state machine */
	if (hub_mgr_data->state == HUB_STATE_OPERATIONAL && hub_mgr_data->current_port > 0) {
		k_work_submit(&hub_mgr_data->port_work.work);
	} else if (hub_mgr_data->state != HUB_STATE_OPERATIONAL) {
		k_work_submit(&hub_mgr_data->hub_work.work);
	}
}

/**
 * @brief Start hub interrupt monitoring
 */
static int usbh_hub_mgr_start_interrupt(struct usbh_hub_mgr_data *hub_mgr_data)
{
	struct uhc_transfer *xfer;
	struct net_buf *buf;
	int ret;

	if (!hub_mgr_data || hub_mgr_data->being_removed || hub_mgr_data->int_active) {
		return -EINVAL;
	}

	/* Start interrupt only when operational */
	if (hub_mgr_data->state != HUB_STATE_OPERATIONAL) {
		return -ENOENT;
	}

	/* Check interrupt endpoint */
	if (!hub_mgr_data->int_ep) {
		LOG_ERR("No interrupt endpoint available");
		return -ENODEV;
	}

	/* Allocate interrupt transfer */
	xfer = usbh_xfer_alloc(hub_mgr_data->hub_udev, hub_mgr_data->int_ep->bEndpointAddress,
			       usbh_hub_mgr_interrupt_in_cb, (void *)hub_mgr_data);
	if (!xfer) {
		LOG_ERR("Failed to allocate interrupt transfer");
		return -ENOMEM;
	}

	/* Allocate receive buffer */
	buf = usbh_xfer_buf_alloc(hub_mgr_data->hub_udev, 
				  sys_le16_to_cpu(hub_mgr_data->int_ep->wMaxPacketSize));
	if (!buf) {
		LOG_ERR("Failed to allocate interrupt buffer");
		usbh_xfer_free(hub_mgr_data->hub_udev, xfer);
		return -ENOMEM;
	}

	xfer->buf = buf;

	ret = usbh_xfer_enqueue(hub_mgr_data->hub_udev, xfer);
	if (ret != 0) {
		LOG_ERR("Failed to enqueue interrupt transfer: %d", ret);
		usbh_xfer_buf_free(hub_mgr_data->hub_udev, buf);
		usbh_xfer_free(hub_mgr_data->hub_udev, xfer);
		return ret;
	}

	hub_mgr_data->interrupt_transfer = xfer;
	hub_mgr_data->int_active = true;
	hub_mgr_data->prime_status = kPrimeInterrupt;
	
	LOG_DBG("Hub level %d interrupt monitoring started", hub_mgr_data->hub_level);
	return 0;
}

/**
 * @brief Find hub manager data by USB device
 */
static struct usbh_hub_mgr_data *find_hub_mgr_by_udev(struct usb_device *udev)
{
	struct usbh_hub_mgr_data *hub_mgr_data;
	
	k_mutex_lock(&hub_mgr.lock, K_FOREVER);
	
	SYS_SLIST_FOR_EACH_CONTAINER(&hub_mgr.hub_list, hub_mgr_data, node) {
		if (hub_mgr_data->hub_udev == udev) {
			k_mutex_unlock(&hub_mgr.lock);
			return hub_mgr_data;
		}
	}
	
	k_mutex_unlock(&hub_mgr.lock);

	return NULL;
}

/**
 * @brief Recursively remove all child hubs
 */
static void usbh_hub_mgr_remove_children_recursive(struct usbh_hub_mgr_data *hub_mgr_data)
{
	struct usbh_hub_mgr_data *child_hub, *next_child;
	
	k_mutex_unlock(&hub_mgr_data->lock);
}

/**
 * @brief Process hub interrupt data
 */
static void usbh_hub_mgr_process_data(struct usbh_hub_mgr_data *hub_mgr_data)
{
	uint8_t port_index;

	k_mutex_lock(&hub_mgr_data->lock, K_FOREVER);

	if (hub_mgr_data->being_removed) {
		k_mutex_unlock(&hub_mgr_data->lock);
		return;
	}

	for (port_index = 0; port_index <= hub_mgr_data->num_ports; ++port_index) {
		if (0 != ((0x01U << (port_index & 0x07U)) & 
			  (hub_mgr_data->int_buffer[port_index >> 3U]))) {
			
			if (port_index == 0) {
				/* Hub status change */
				LOG_DBG("Hub %d status changed", hub_mgr_data->hub_level);
				continue;
			} else {
				if ((hub_mgr.current_hub_process == NULL) ||
				    (hub_mgr.current_hub_process == hub_mgr_data)) {
					
					hub_mgr.current_hub_process = hub_mgr_data;
					hub_mgr_data->current_port = port_index;
					
					LOG_INF("Hub level %d port %d status changed, starting processing", 
						hub_mgr_data->hub_level, port_index);
					
					k_mutex_unlock(&hub_mgr_data->lock);
					
					k_work_submit(&hub_mgr_data->port_work.work);
					return;
				}
			}
		}
	}

	k_mutex_unlock(&hub_mgr_data->lock);

	if (!hub_mgr_data->int_active && !hub_mgr_data->being_removed) {
		usbh_hub_mgr_start_interrupt(hub_mgr_data);
	}
}

/**
 * @brief Hub interrupt IN callback
 */
static int usbh_hub_mgr_interrupt_in_cb(struct usb_device *const dev,
					 struct uhc_transfer *const xfer)
{
	struct usbh_hub_mgr_data *hub_mgr_data = (struct usbh_hub_mgr_data *)xfer->priv;
	struct net_buf *buf = xfer->buf;

	if (!hub_mgr_data || hub_mgr_data->being_removed) {
		goto cleanup_and_exit;
	}

	hub_mgr_data->int_active = false;

	hub_mgr_data->prime_status = kPrimeNone;

	if (!buf || buf->len == 0) {
		LOG_ERR("Hub level %d interrupt transfer failed or no data", 
			hub_mgr_data->hub_level);
		if (!hub_mgr_data->control_transfer) {
			hub_mgr.current_hub_process = NULL;
			hub_mgr_data->current_port = 0;
		}
		goto resubmit;
	}

	memcpy(hub_mgr_data->int_buffer, buf->data, 
	       MIN(buf->len, sizeof(hub_mgr_data->int_buffer)));

	LOG_DBG("Hub level %d interrupt data received: length=%d", 
		hub_mgr_data->hub_level, buf->len);

	usbh_hub_mgr_process_data(hub_mgr_data);

	net_buf_unref(buf);
	return 0;

resubmit:
	/* Resubmit transfer */
	if (hub_mgr_data->connected && hub_mgr_data->state == HUB_STATE_OPERATIONAL) {
		int ret = usbh_hub_mgr_resubmit_interrupt_in(hub_mgr_data, xfer);
		if (ret) {
			LOG_ERR("Failed to resubmit interrupt transfer: %d", ret);
		}
	} else {
		usbh_xfer_free(hub_mgr_data->hub_udev, xfer);
	}
	return 0;

cleanup_and_exit:
	/* Cleanup resources */
	if (buf) {
		net_buf_unref(buf);
	}
	usbh_xfer_free(dev, xfer);
	return 0;
}

/**
 * @brief Submit interrupt IN transfer for hub status monitoring
 */
int usbh_hub_submit_interrupt_in(struct usbh_hub_instance *hub_instance,
				 struct usb_ep_descriptor *int_ep,
				 int (*callback)(struct usb_device *const dev,
						struct uhc_transfer *const xfer),
				 void *callback_param)
{
	struct uhc_transfer *xfer;
	struct net_buf *buf;
	int ret;

	if (!hub_instance || !hub_instance->hub_udev || !int_ep) {
		return -ENODEV;
	}

	/* Allocate USB transfer */
	xfer = usbh_xfer_alloc(hub_instance->hub_udev, int_ep->bEndpointAddress,
			       callback, callback_param);
	if (!xfer) {
		LOG_ERR("Failed to allocate interrupt IN transfer");
		return -ENOMEM;
	}

	/* Allocate transfer buffer */
	buf = usbh_xfer_buf_alloc(hub_instance->hub_udev, 
				  sys_le16_to_cpu(int_ep->wMaxPacketSize));
	if (!buf) {
		LOG_ERR("Failed to allocate interrupt IN buffer");
		usbh_xfer_free(hub_instance->hub_udev, xfer);
		return -ENOMEM;
	}

	xfer->buf = buf;

	/* Submit transfer */
	ret = usbh_xfer_enqueue(hub_instance->hub_udev, xfer);
	if (ret != 0) {
		LOG_ERR("Failed to enqueue interrupt IN transfer: %d", ret);
		usbh_xfer_buf_free(hub_instance->hub_udev, buf);
		usbh_xfer_free(hub_instance->hub_udev, xfer);
		return ret;
	}

	LOG_DBG("Hub interrupt IN transfer submitted");
	return 0;
}

/**
 * @brief Hub process state machine
 */
static void usbh_hub_mgr_process(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct usbh_hub_mgr_data *hub_mgr_data = 
		CONTAINER_OF(dwork, struct usbh_hub_mgr_data, hub_work);
	struct usb_hub_descriptor *hub_desc;
	uint8_t need_prime_interrupt = 0;
	uint8_t process_success = 0;
	uint16_t port_num;
	int ret;

	k_mutex_lock(&hub_mgr_data->lock, K_FOREVER);

	if (hub_mgr_data->being_removed) {
		k_mutex_unlock(&hub_mgr_data->lock);
		return;
	}

	switch (hub_mgr_data->hub_status) {
	case kHubRunIdle:
		/* Hub idle, process hub status change */
		if (hub_mgr_data->prime_status == kPrimeHubControl) {
			LOG_DBG("Processing Hub status change");
			hub_mgr_data->hub_status = kHubRunClearDone;
			process_success = 1;
			need_prime_interrupt = 1;
		}
		break;

	case kHubRunInvalid:
		LOG_ERR("Hub in invalid state");
		hub_mgr_data->state = HUB_STATE_ERROR;
		break;

	case kHubRunWaitSetInterface:
		hub_mgr_data->hub_status = kHubRunGetDescriptor7;
		/* Get basic hub descriptor */
		ret = usbh_hub_get_descriptor(&hub_mgr_data->hub_instance,
					      hub_mgr_data->hub_instance.hub_desc_buf, 7,
					      usbh_hub_control_callback, hub_mgr_data);
		if (ret == 0) {
			process_success = 1;
			LOG_DBG("Getting 7-byte hub descriptor");
		} else {
			LOG_ERR("Failed to get hub descriptor: %d", ret);
			hub_mgr_data->hub_status = kHubRunInvalid;
		}
		break;

	case kHubRunGetDescriptor7:
		hub_desc = (struct usb_hub_descriptor *)hub_mgr_data->hub_instance.hub_desc_buf;
		
		/* Save hub properties */
		hub_mgr_data->hub_instance.num_ports = hub_desc->bNbrPorts;
		hub_mgr_data->num_ports = hub_desc->bNbrPorts;
		
		if (hub_mgr_data->num_ports > USB_HUB_MAX_PORTS) {
			LOG_ERR("Too many ports: %d", hub_mgr_data->num_ports);
			hub_mgr_data->hub_status = kHubRunInvalid;
			break;
		}

		LOG_INF("Hub has %d ports", hub_mgr_data->num_ports);

		hub_mgr_data->hub_status = kHubRunSetPortPower;
		/* Get full hub descriptor */
		port_num = (hub_mgr_data->num_ports + 7) / 8;
		ret = usbh_hub_get_descriptor(&hub_mgr_data->hub_instance,
					      hub_mgr_data->hub_instance.hub_desc_buf,
					      7 + port_num,
					      usbh_hub_control_callback, hub_mgr_data);
		if (ret == 0) {
			process_success = 1;
			LOG_DBG("Getting full hub descriptor");
		} else {
			LOG_ERR("Failed to get full hub descriptor: %d", ret);
			hub_mgr_data->hub_status = kHubRunInvalid;
		}
		break;

	case kHubRunSetPortPower:
		/* Allocate port list if not already done */
		if (hub_mgr_data->port_list == NULL) {
			hub_mgr_data->port_list = k_malloc(hub_mgr_data->num_ports * 
							   sizeof(struct usbh_hub_port_instance));
			if (hub_mgr_data->port_list == NULL) {
				LOG_ERR("Failed to allocate port list");
				hub_mgr_data->hub_status = kHubRunInvalid;
				break;
			}
			hub_mgr_data->port_index = 0;
		}

		/* Power on all ports */
		if (hub_mgr_data->port_index < hub_mgr_data->num_ports) {
			hub_mgr_data->port_index++;
			ret = usbh_hub_set_port_feature(&hub_mgr_data->hub_instance,
							 hub_mgr_data->port_index,
							 USB_HUB_FEATURE_PORT_POWER,
							 usbh_hub_control_callback,
							 hub_mgr_data);
			if (ret == 0) {
				process_success = 1;
				LOG_DBG("Setting port %d power", hub_mgr_data->port_index);
			} else {
				LOG_ERR("Failed to set port %d power: %d", 
					hub_mgr_data->port_index, ret);
				need_prime_interrupt = 1;
			}
			break;
		}

		/* All ports powered, initialize port states */
		for (int i = 0; i < hub_mgr_data->num_ports; i++) {
			/* Initialize port instance */
			hub_mgr_data->port_list[i].udev = NULL;
			hub_mgr_data->port_list[i].reset_count = 3;
			hub_mgr_data->port_list[i].port_status = kPortRunWaitPortChange;
			hub_mgr_data->port_list[i].state = PORT_STATE_DISCONNECTED;
			hub_mgr_data->port_list[i].port_num = i + 1;
			hub_mgr_data->port_list[i].speed = USB_SPEED_SPEED_FS; /* Default full speed */
		}
		
		hub_mgr_data->hub_status = kHubRunIdle;
		hub_mgr_data->state = HUB_STATE_OPERATIONAL;
		need_prime_interrupt = 1;
		LOG_INF("Hub initialization completed, starting interrupt monitoring");
		break;

	case kHubRunGetStatusDone:
		hub_mgr_data->hub_status = kHubRunClearDone;
		LOG_DBG("Hub status get completed");
		need_prime_interrupt = 1;
		process_success = 1;
		break;

	case kHubRunClearDone:
		hub_mgr_data->hub_status = kHubRunIdle;
		need_prime_interrupt = 1;
		process_success = 1;
		break;

	default:
		LOG_ERR("Unknown hub status: %d", hub_mgr_data->hub_status);
		hub_mgr_data->hub_status = kHubRunInvalid;
		hub_mgr_data->state = HUB_STATE_ERROR;
		break;
	}

	k_mutex_unlock(&hub_mgr_data->lock);

	if (need_prime_interrupt) {
		hub_mgr_data->hub_status = kHubRunIdle;
		if (!hub_mgr_data->int_active && !hub_mgr_data->being_removed) {
			ret = usbh_hub_mgr_start_interrupt(hub_mgr_data);
			if (ret) {
				LOG_ERR("Failed to start interrupt monitoring: %d", ret);
			}
		}
	} else if (!process_success && hub_mgr_data->hub_status != kHubRunInvalid) {
		hub_mgr_data->hub_status = kHubRunInvalid;
		hub_mgr_data->state = HUB_STATE_ERROR;
	}
}

/**
 * @brief Recursively disconnect hub and all children
 */
static void usbh_hub_mgr_recursive_disconnect(struct usbh_hub_mgr_data *hub_mgr_data)
{
	if (!hub_mgr_data) {
		return;
	}

	k_mutex_lock(&hub_mgr_data->lock, K_FOREVER);

	if (hub_mgr_data->being_removed) {
		k_mutex_unlock(&hub_mgr_data->lock);
		LOG_DBG("Hub level %d already being removed", hub_mgr_data->hub_level);
		return;
	}

	hub_mgr_data->being_removed = true;

	LOG_INF("Recursively disconnecting Hub level %d and all children", 
		hub_mgr_data->hub_level);

	k_work_cancel_delayable(&hub_mgr_data->port_work);
	k_work_cancel_delayable(&hub_mgr_data->hub_work);
	hub_mgr_data->int_active = false;

	if (hub_mgr.current_hub_process == hub_mgr_data) {
		hub_mgr.current_hub_process = NULL;
		hub_mgr_data->current_port = 0;
	}

	for (int i = 0; i < hub_mgr_data->num_ports; i++) {
		if (hub_mgr_data->port_list && hub_mgr_data->port_list[i].udev) {
			struct usb_device *port_udev = hub_mgr_data->port_list[i].udev;
			struct usbh_hub_mgr_data *child_hub = find_hub_mgr_by_udev(port_udev);
			if (child_hub) {
				LOG_INF("Found child Hub on port %d, recursing", i + 1);
				k_mutex_unlock(&hub_mgr_data->lock);
				
				usbh_hub_mgr_recursive_disconnect(child_hub);
				
				k_mutex_lock(&hub_mgr_data->lock, K_FOREVER);
			} else {
				LOG_INF("Disconnecting device on port %d", i + 1);
				
				usbh_device_disconnected(hub_mgr_data->uhs_ctx, port_udev);
			}
			
			hub_mgr_data->port_list[i].udev = NULL;
			hub_mgr_data->port_list[i].state = PORT_STATE_DISCONNECTED;
		}
	}

	k_mutex_unlock(&hub_mgr_data->lock);

	LOG_INF("Triggering Hub level %d removal", hub_mgr_data->hub_level);

	usbh_device_disconnected(hub_mgr_data->uhs_ctx, hub_mgr_data->hub_udev);
}

/**
 * @brief Establish parent-child hub relationship
 */
static void establish_parent_child_relationship(struct usbh_hub_mgr_data *parent_hub, 
					uint8_t port_num, 
					struct usb_device *child_udev)
{
	struct usbh_hub_mgr_data *child_hub = find_hub_mgr_by_udev(child_udev);
	if (child_hub && !child_hub->parent_hub) {
		child_hub->parent_hub = parent_hub;
		child_hub->parent_port = port_num;
		child_hub->hub_level = parent_hub->hub_level + 1;

		sys_slist_append(&parent_hub->child_hubs, &child_hub->child_node);

		LOG_INF("Hub parent-child relationship established: parent level %d -> child level %d", 
			parent_hub->hub_level, child_hub->hub_level);
	}
}

/**
 * @brief Port processing state machine
 */
static void usbh_hub_port_process(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct usbh_hub_mgr_data *hub_mgr_data = 
		CONTAINER_OF(dwork, struct usbh_hub_mgr_data, port_work);
	struct usbh_hub_port_instance *port_instance;
	uint8_t port_num = hub_mgr_data->current_port;
	uint32_t spec_status;
	uint8_t feature = 0;
	int ret;
	bool need_restart_interrupt = false;
	bool process_complete = false;

	if (port_num == 0 || port_num > hub_mgr_data->num_ports || !hub_mgr_data->port_list) {
		LOG_ERR("Invalid port state for processing");
		goto exit_processing;
	}

	port_instance = &hub_mgr_data->port_list[port_num - 1];

	k_mutex_lock(&hub_mgr_data->lock, K_FOREVER);

	if (hub_mgr_data->being_removed) {
		k_mutex_unlock(&hub_mgr_data->lock);
		return;
	}

	/* Error frequency control */
	uint32_t current_time = k_uptime_get_32();
	if (current_time - hub_mgr_data->last_error_time < 3000) {
		hub_mgr_data->error_count++;
		if (hub_mgr_data->error_count > 5) {
			LOG_ERR("Hub level %d port %d: Too many errors, suspending", 
				hub_mgr_data->hub_level, port_num);
			port_instance->port_status = kPortRunInvalid;
			k_work_reschedule(&hub_mgr_data->port_work, K_MSEC(10000));
			hub_mgr_data->error_count = 0;
			k_mutex_unlock(&hub_mgr_data->lock);
			return;
		}
	} else {
		hub_mgr_data->error_count = 0;
	}

	/* Device state check */
	if (!hub_mgr_data->hub_udev || hub_mgr_data->hub_udev->state != USB_STATE_CONFIGURED) {
		LOG_ERR("Hub device not ready");
		hub_mgr_data->last_error_time = current_time;
		k_work_reschedule(&hub_mgr_data->port_work, K_MSEC(5000));
		k_mutex_unlock(&hub_mgr_data->lock);
		return;
	}

	k_mutex_unlock(&hub_mgr_data->lock);

	LOG_DBG("Processing port %d, status=%d", port_num, port_instance->port_status);

	/* Port state machine main logic */
	switch (port_instance->port_status) {
	case kPortRunWaitPortChange:
		/* Step 1: Get port status */
		LOG_DBG("Port %d: Getting port status", port_num);
		port_instance->port_status = kPortRunCheckCPortConnection;
		ret = usbh_hub_get_port_status(&hub_mgr_data->hub_instance, port_num,
					       (uint8_t*)&hub_mgr_data->hub_instance.port_status,
					       sizeof(hub_mgr_data->hub_instance.port_status),
					       usbh_hub_control_callback, hub_mgr_data);
		if (ret != 0) {
			LOG_ERR("Failed to get port status: %d", ret);
			goto error_recovery;
		}
		/* Wait for control transfer completion, callback will continue processing */
		return;

	case kPortRunCheckCPortConnection:
		/* Step 2: Analyze port status and changes */
		spec_status = ((uint32_t)hub_mgr_data->hub_instance.port_status.wPortChange << 16) |
			      hub_mgr_data->hub_instance.port_status.wPortStatus;

		LOG_INF("Port %d status: wPortStatus=0x%04x, wPortChange=0x%04x", 
			port_num, 
			hub_mgr_data->hub_instance.port_status.wPortStatus,
			hub_mgr_data->hub_instance.port_status.wPortChange);

		if (spec_status & (1UL << USB_HUB_FEATURE_C_PORT_CONNECTION)) {
			/* Connection status change */
			port_instance->port_status = kPortRunGetPortConnection;
			ret = usbh_hub_clear_port_feature(&hub_mgr_data->hub_instance, port_num,
							  USB_HUB_FEATURE_C_PORT_CONNECTION,
							  usbh_hub_control_callback, hub_mgr_data);
			if (ret != 0) {
				LOG_ERR("Failed to clear port connection change: %d", ret);
				goto error_recovery;
			}
			LOG_DBG("Port %d: Cleared connection change bit", port_num);
			return;

		} else if (spec_status & (1UL << USB_HUB_FEATURE_PORT_CONNECTION)) {
			/* Device connected, need reset */
			LOG_ERR("Device connected to port %d, starting reset", port_num);
			port_instance->port_status = kPortRunWaitPortResetDone;
			ret = usbh_hub_set_port_feature(&hub_mgr_data->hub_instance, port_num,
							USB_HUB_FEATURE_PORT_RESET,
							usbh_hub_control_callback, hub_mgr_data);
			if (ret != 0) {
				LOG_ERR("Failed to reset port: %d", ret);
				goto error_recovery;
			}
			if (port_instance->reset_count > 0) {
				port_instance->reset_count--;
			}
			return;

		} else if (spec_status & (1UL << (USB_HUB_FEATURE_C_PORT_RESET))) {
			/* Reset completed */
			feature = USB_HUB_FEATURE_C_PORT_RESET;
			port_instance->port_status = kPortRunCheckCPortReset;

		} else if (spec_status & (1UL << (USB_HUB_FEATURE_C_PORT_ENABLE))) {
			/* Enable status change */
			feature = USB_HUB_FEATURE_C_PORT_ENABLE;
			port_instance->port_status = kPortRunWaitPortChange;

		} else if (spec_status & (1UL << (USB_HUB_FEATURE_C_PORT_OVER_CURRENT))) {
			/* Over-current detection */
			feature = USB_HUB_FEATURE_C_PORT_OVER_CURRENT;
			port_instance->port_status = kPortRunWaitPortChange;
			LOG_WRN("Port %d over-current detected", port_num);

		} else if (!(spec_status & (1UL << USB_HUB_FEATURE_PORT_CONNECTION))) {
			/* Device disconnected */
			goto process_disconnection;
		}

		if (feature != 0) {
			ret = usbh_hub_clear_port_feature(&hub_mgr_data->hub_instance, port_num,
							  feature, usbh_hub_control_callback, hub_mgr_data);
			if (ret != 0) {
				LOG_ERR("Failed to clear feature %d: %d", feature, ret);
				goto error_recovery;
			}
			LOG_DBG("Port %d: Cleared feature %d", port_num, feature);
			return;
		}
		break;

	case kPortRunGetPortConnection:
		/* Step 3: Get status again to confirm connection */
		port_instance->port_status = kPortRunCheckPortConnection;
		ret = usbh_hub_get_port_status(&hub_mgr_data->hub_instance, port_num,
					       (uint8_t*)&hub_mgr_data->hub_instance.port_status,
					       sizeof(hub_mgr_data->hub_instance.port_status),
					       usbh_hub_control_callback, hub_mgr_data);
		if (ret != 0) {
			LOG_ERR("Failed to get port connection status: %d", ret);
			goto error_recovery;
		}
		return;

	case kPortRunCheckPortConnection:
		/* Step 4: Check connection status */
		spec_status = ((uint32_t)hub_mgr_data->hub_instance.port_status.wPortChange << 16) |
			      hub_mgr_data->hub_instance.port_status.wPortStatus;

		if (spec_status & (1UL << USB_HUB_FEATURE_PORT_CONNECTION)) {
			/* Confirm connection, start reset */
			LOG_INF("Port %d connection confirmed, resetting", port_num);
			port_instance->port_status = kPortRunWaitPortResetDone;
			ret = usbh_hub_set_port_feature(&hub_mgr_data->hub_instance, port_num,
							USB_HUB_FEATURE_PORT_RESET,
							usbh_hub_control_callback, hub_mgr_data);
			if (ret != 0) {
				LOG_ERR("Failed to reset port: %d", ret);
				goto error_recovery;
			}
			if (port_instance->reset_count > 0) {
				port_instance->reset_count--;
			}
			return;
		} else {
			/* Device disconnected */
			goto process_disconnection;
		}
		break;

	case kPortRunWaitPortResetDone:
		/* Step 5: Wait for reset completion */
		port_instance->port_status = kPortRunWaitCPortReset;
		/* Wait for interrupt notification of reset completion, restart interrupt listening */
		if (!hub_mgr_data->int_active && !hub_mgr_data->being_removed) {
			ret = usbh_hub_mgr_start_interrupt(hub_mgr_data);
			if (ret) {
				LOG_ERR("Failed to restart interrupt for reset wait: %d", ret);
			}
		}
		LOG_DBG("Port %d waiting for reset completion interrupt", port_num);
		return;

	case kPortRunWaitCPortReset:
		/* Step 6: Get port status to check reset completion */
		port_instance->port_status = kPortRunCheckCPortReset;
		ret = usbh_hub_get_port_status(&hub_mgr_data->hub_instance, port_num,
					       (uint8_t*)&hub_mgr_data->hub_instance.port_status,
					       sizeof(hub_mgr_data->hub_instance.port_status),
					       usbh_hub_control_callback, hub_mgr_data);
		if (ret != 0) {
			LOG_ERR("Failed to get port status for reset check: %d", ret);
			goto error_recovery;
		}
		return;

	case kPortRunCheckCPortReset:
		/* Step 7: Check reset completion flag and clear */
		spec_status = ((uint32_t)hub_mgr_data->hub_instance.port_status.wPortChange << 16) |
				hub_mgr_data->hub_instance.port_status.wPortStatus;

		if (spec_status & (1UL << USB_HUB_FEATURE_C_PORT_RESET)) {
			if (port_instance->reset_count == 0) {
				/* Reset completed, device connected */
				port_instance->port_status = kPortRunPortAttached;
				
				/* Detect device speed */
				if (spec_status & (1UL << USB_HUB_FEATURE_PORT_HIGH_SPEED)) {
					port_instance->speed = USB_SPEED_SPEED_HS;
				} else if (spec_status & (1UL << USB_HUB_FEATURE_PORT_LOW_SPEED)) {
					port_instance->speed = USB_SPEED_SPEED_FS;
				} else {
					port_instance->speed = USB_SPEED_SPEED_FS;
				}
				
				LOG_INF("Device ready on port %d (speed: %s)", port_num,
					port_instance->speed == USB_SPEED_SPEED_HS ? "HIGH" :
					port_instance->speed == USB_SPEED_SPEED_FS ? "LOW" : "FULL");
			} else {
				/* Need another reset */
				port_instance->port_status = kPortRunResetAgain;
			}
			
			/* Clear C_PORT_RESET */
			ret = usbh_hub_clear_port_feature(&hub_mgr_data->hub_instance, port_num,
							USB_HUB_FEATURE_C_PORT_RESET,
							usbh_hub_control_callback, hub_mgr_data);
			if (ret != 0) {
				LOG_ERR("Failed to clear port reset feature: %d", ret);
				goto error_recovery;
			}
			return;
		} else {
			/* Reset not completed, check again */
			LOG_DBG("Port %d reset not completed, checking again", port_num);
			port_instance->port_status = kPortRunWaitCPortReset;
			k_work_reschedule(&hub_mgr_data->port_work, K_MSEC(100));
			return;
		}
		break;

	case kPortRunResetAgain:
		/* Step 8: Reset again */
		port_instance->port_status = kPortRunCheckPortConnection;
		ret = usbh_hub_get_port_status(&hub_mgr_data->hub_instance, port_num,
					       (uint8_t*)&hub_mgr_data->hub_instance.port_status,
					       sizeof(hub_mgr_data->hub_instance.port_status),
					       usbh_hub_control_callback, hub_mgr_data);
		if (ret != 0) {
			LOG_ERR("Failed to get port status for reset again: %d", ret);
			goto error_recovery;
		}
		return;

	case kPortRunPortAttached:
		LOG_INF("Device attached to port %d", port_num);
		
		/* Notify USB host stack to start device enumeration */
		struct usb_device *new_udev = host_connect_device(hub_mgr_data->uhs_ctx, port_instance->speed);
		if (new_udev != NULL) {
			/* Save device to port instance */
			port_instance->udev = new_udev;
			
			LOG_INF("Device enumeration completed for port %d, addr=%d", 
				port_num, new_udev->addr);
			
			/* Delay check for child hub relationship */
			port_instance->port_status = kPortRunCheckChildHub;
			k_work_reschedule(&hub_mgr_data->port_work, K_MSEC(200));
			return;
			
		} else {
			LOG_ERR("Device enumeration failed for port %d", port_num);
			
			/* Retry enumeration */
			if (port_instance->reset_count > 0) {
				port_instance->reset_count--;
				port_instance->port_status = kPortRunWaitPortChange;
				LOG_WRN("Port %d enumeration failed, %d retries left", 
						port_num, port_instance->reset_count);
				k_work_reschedule(&hub_mgr_data->port_work, K_MSEC(1000));
				return;
			} else {
				LOG_ERR("Port %d enumeration max retries exceeded", port_num);
			}
		}
		
		process_complete = true;
		hub_mgr_data->current_port = 0;
		hub_mgr.current_hub_process = NULL;
		port_instance->reset_count = 3;
		need_restart_interrupt = true;
		goto exit_processing;

	case kPortRunCheckChildHub:
		/* Check and establish parent-child hub relationship */
		if (port_instance->udev) {
			establish_parent_child_relationship(hub_mgr_data, port_num, port_instance->udev);
		}
		
		/* Complete port processing */
		process_complete = true;
		hub_mgr_data->current_port = 0;
		hub_mgr.current_hub_process = NULL;
		port_instance->reset_count = 3;
		need_restart_interrupt = true;
		goto exit_processing;

	default:
		LOG_ERR("Unknown port status: %d", port_instance->port_status);
		goto error_recovery;
	}

	/* Process device disconnection */
process_disconnection:
	if (port_instance->udev) {
		struct usb_device *disconnected_udev = port_instance->udev;
		
		LOG_INF("Device disconnected from Hub level %d port %d", 
			hub_mgr_data->hub_level, port_num);
		
		/* Clear port state immediately */
		port_instance->udev = NULL;
		port_instance->state = PORT_STATE_DISCONNECTED;
		
		if (disconnected_udev) {
			struct usbh_hub_mgr_data *child_hub = find_hub_mgr_by_udev(disconnected_udev);
			if (child_hub) {
				LOG_INF("Child Hub disconnected, triggering recursive removal");
				
				usbh_hub_mgr_recursive_disconnect(child_hub);
			} else {
				/* Normal device disconnect */
				usbh_device_disconnected(hub_mgr_data->uhs_ctx, disconnected_udev);
			}
		}
	}
	process_complete = true;

exit_processing:
	/* Processing completed, cleanup state */
	if (process_complete || port_instance->port_status == kPortRunInvalid) {
		hub_mgr_data->current_port = 0;
		hub_mgr.current_hub_process = NULL;
		port_instance->port_status = kPortRunWaitPortChange;
		port_instance->reset_count = 3; /* Reset retry count */
		need_restart_interrupt = true;
		
		LOG_DBG("Port %d processing completed", port_num);
	}

	if (need_restart_interrupt && !hub_mgr_data->int_active && !hub_mgr_data->being_removed) {
		ret = usbh_hub_mgr_start_interrupt(hub_mgr_data);
		if (ret) {
			LOG_ERR("Failed to restart interrupt monitoring: %d", ret);
		}
	}
	
	hub_mgr_data->error_count = 0;
	return;

error_recovery:
	hub_mgr_data->last_error_time = current_time;

	if (port_instance->reset_count > 0) {
		port_instance->reset_count--;
		port_instance->port_status = kPortRunWaitPortChange;
		
		LOG_WRN("Port %d error recovery, %d retries left", port_num, port_instance->reset_count);
		k_work_reschedule(&hub_mgr_data->port_work, K_MSEC(3000));
	} else {
		LOG_ERR("Port %d max retries exceeded, disabling", port_num);
		port_instance->port_status = kPortRunInvalid;
		goto exit_processing;
	}
}

/**
 * @brief USBH class implementation for HUB devices
 */
static int usbh_hub_mgr_probe(struct usbh_class_data *const c_data,
			  struct usb_device *const udev,
			  const uint8_t iface)
{
	struct usbh_hub_mgr_data *hub_mgr_data;
	const void *desc_start;
	const void *desc_end;
	const void *cfg_end;
	int ret;

	LOG_INF("USB HUB device probe at interface %u", iface);

	/* Get descriptor range for current interface */
	cfg_end = usbh_desc_get_cfg_end(udev);
	desc_start = usbh_desc_get_by_iface(usbh_desc_get_cfg_beg(udev), cfg_end, iface);
	if (desc_start == NULL) {
		LOG_ERR("Failed to find interface %u descriptor", iface);
		return -ENODEV;
	}

	/* Get the start of next function as the end of current function */
	desc_end = usbh_desc_get_next_function(desc_start, cfg_end);
	if (desc_end == NULL) {
		/* If no next function exists, use configuration descriptor end */
		desc_end = cfg_end;
	}

	hub_mgr_data = k_malloc(sizeof(*hub_mgr_data));
	if (!hub_mgr_data) {
		LOG_ERR("Failed to allocate HUB management data");
		return -ENOMEM;
	}

	memset(hub_mgr_data, 0, sizeof(*hub_mgr_data));
	
	ret = usbh_hub_init_instance(&hub_mgr_data->hub_instance, udev);
	if (ret) {
		LOG_ERR("Failed to initialize HUB instance: %d", ret);
		k_free(hub_mgr_data);
		return ret;
	}

	hub_mgr_data->hub_udev = udev;
	hub_mgr_data->uhs_ctx = c_data->uhs_ctx;
	hub_mgr_data->state = HUB_STATE_INIT;
	hub_mgr_data->hub_status = kHubRunWaitSetInterface;
	hub_mgr_data->port_index = 0;
	hub_mgr_data->port_list = NULL;
	hub_mgr_data->being_removed = false;

	hub_mgr_data->prime_status = kPrimeNone;
	hub_mgr_data->control_transfer = NULL;
	hub_mgr_data->interrupt_transfer = NULL;
	hub_mgr_data->int_active = false;

	sys_slist_init(&hub_mgr_data->child_hubs);

	/* Parse interrupt endpoint within the correct descriptor range */
	uint8_t *desc_buf = (uint8_t *)desc_start;
	uint8_t *desc_end_buf = (uint8_t *)desc_end;

	while (desc_buf < desc_end_buf) {
		struct usb_desc_header *header = (struct usb_desc_header *)desc_buf;

		if (header->bLength == 0) {
			break;
		}

		if (header->bDescriptorType == USB_DESC_ENDPOINT) {
			struct usb_ep_descriptor *ep_desc = (struct usb_ep_descriptor *)desc_buf;
			
			if ((ep_desc->bEndpointAddress & USB_EP_DIR_MASK) == USB_EP_DIR_IN &&
			    (ep_desc->bmAttributes & USB_EP_TRANSFER_TYPE_MASK) == USB_EP_TYPE_INTERRUPT) {
				hub_mgr_data->int_ep = ep_desc;
				LOG_DBG("Found hub interrupt IN endpoint 0x%02x", 
					ep_desc->bEndpointAddress);
				break;
			}
		}

		desc_buf += header->bLength;
	}

	hub_mgr_data->connected = true;
	hub_mgr_data->int_active = false;

	k_mutex_init(&hub_mgr_data->lock);
	k_work_init_delayable(&hub_mgr_data->hub_work, usbh_hub_mgr_process);
	k_work_init_delayable(&hub_mgr_data->port_work, usbh_hub_port_process);

	if (udev->addr == 1) {
		hub_mgr_data->hub_level = 0;
		hub_mgr_data->parent_hub = NULL;
		hub_mgr_data->parent_port = 0;
		LOG_INF("Root Hub connected (addr=1)");
	} else {
		hub_mgr_data->hub_level = 1;
		hub_mgr_data->parent_hub = NULL;
		hub_mgr_data->parent_port = 1;
		LOG_INF("Child Hub connected (addr=%d)", udev->addr);
	}

	c_data->priv = hub_mgr_data;

	k_mutex_lock(&hub_mgr.lock, K_FOREVER);
	sys_slist_append(&hub_mgr.hub_list, &hub_mgr_data->node);
	hub_mgr.total_hubs++;
	k_mutex_unlock(&hub_mgr.lock);

	k_work_submit(&hub_mgr_data->hub_work.work);

	LOG_INF("Hub probe successful (level %d)", hub_mgr_data->hub_level);
	return 0;
}

/**
 * @brief USBH class implementation for HUB device removal
 */
static int usbh_hub_mgr_removed(struct usb_device *udev, struct usbh_class_data *cdata)
{
	struct usbh_hub_mgr_data *hub_mgr_data = cdata->priv;

	LOG_INF("USB HUB device removed");

	if (!hub_mgr_data) {
		return 0;
	}

	/* Mark as being removed to prevent duplicate processing */
	k_mutex_lock(&hub_mgr_data->lock, K_FOREVER);
	if (hub_mgr_data->being_removed) {
		k_mutex_unlock(&hub_mgr_data->lock);
		LOG_DBG("Hub already being removed");
		return 0;
	}
	hub_mgr_data->being_removed = true;
	k_mutex_unlock(&hub_mgr_data->lock);

	/* Release global processing token */
	if (hub_mgr.current_hub_process == hub_mgr_data) {
		hub_mgr.current_hub_process = NULL;
	}

	/* Recursively remove all child hubs */
	usbh_hub_mgr_remove_children_recursive(hub_mgr_data);

	/* Cancel all work items */
	k_work_cancel_delayable(&hub_mgr_data->hub_work);
	k_work_cancel_delayable(&hub_mgr_data->port_work);

	/* Cancel interrupt transfer */
	if (hub_mgr_data->interrupt_transfer && hub_mgr_data->int_active) {
		hub_mgr_data->interrupt_transfer = NULL;
		hub_mgr_data->int_active = false;
		LOG_DBG("Interrupt transfer cancelled");
	}

	/* Cancel control transfer */
	if (hub_mgr_data->control_transfer) {
		hub_mgr_data->control_transfer = NULL;
	}

	/* Remove all connected devices */
	for (int i = 0; i < hub_mgr_data->num_ports; i++) {
		if (hub_mgr_data->port_list) {
			if (hub_mgr_data->port_list[i].udev) {
				hub_mgr_data->port_list[i].udev = NULL;
				hub_mgr_data->port_list[i].state = PORT_STATE_DISCONNECTED;
			}
		}
	}

	/* Remove from parent hub's child hub list */
	if (hub_mgr_data->parent_hub) {
		k_mutex_lock(&hub_mgr_data->parent_hub->lock, K_FOREVER);
		sys_slist_find_and_remove(&hub_mgr_data->parent_hub->child_hubs, &hub_mgr_data->child_node);
		k_mutex_unlock(&hub_mgr_data->parent_hub->lock);
	}

	/* Remove from global list */
	k_mutex_lock(&hub_mgr.lock, K_FOREVER);
	sys_slist_find_and_remove(&hub_mgr.hub_list, &hub_mgr_data->node);
	if (hub_mgr.total_hubs > 0) {
		hub_mgr.total_hubs--;
	}
	k_mutex_unlock(&hub_mgr.lock);

	/* Cleanup hub instance */
	usbh_hub_cleanup_instance(&hub_mgr_data->hub_instance);

	/* Free port list */
	if (hub_mgr_data->port_list) {
		k_free(hub_mgr_data->port_list);
		hub_mgr_data->port_list = NULL;
	}

	k_free(hub_mgr_data);
	cdata->priv = NULL;

	LOG_INF("Hub removal completed");
	return 0;
}

/**
 * @brief Hub class initialization function
 */
static int usbh_hub_mgr_class_init(struct usbh_class_data *const c_data, struct usbh_context *const uhs_ctx)
{
	c_data->uhs_ctx = uhs_ctx;
	return 0;
}

static const struct usbh_class_filter hub_filters[] = {{
		.flags = USBH_CLASS_MATCH_CLASS | USBH_CLASS_MATCH_SUB,
		.class = USB_HUB_CLASS_CODE,
		.sub = USB_HUB_SUBCLASS_CODE,
}};

/* Hub class API structure */
static struct usbh_class_api usbh_hub_class_api = {
	.init = usbh_hub_mgr_class_init,
	.completion_cb = NULL,
	.probe = usbh_hub_mgr_probe,
	.removed = usbh_hub_mgr_removed,
	.suspended = NULL,
	.resumed = NULL,
};

/**
 * @brief Initialize HUB manager
 */
static int usbh_hub_mgr_init(void)
{
	sys_slist_init(&hub_mgr.hub_list);
	k_mutex_init(&hub_mgr.lock);
	hub_mgr.total_hubs = 0;
	hub_mgr.max_concurrent_hubs = 0;
	hub_mgr.current_hub_process = NULL;
	hub_mgr.uhs_ctx = NULL;
	
	LOG_INF("USB HUB manager initialized");
	return 0;
}

SYS_INIT(usbh_hub_mgr_init, POST_KERNEL, CONFIG_USBH_INIT_PRIO);

/* Register Hub class driver */
USBH_DEFINE_CLASS(usbh_hub_class, &usbh_hub_class_api, NULL,
		  hub_filters, ARRAY_SIZE(hub_filters));
