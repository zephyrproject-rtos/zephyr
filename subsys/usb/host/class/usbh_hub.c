/*
 * SPDX-FileCopyrightText: Copyright 2025 - 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/sys/byteorder.h>
#include "usbh_class.h"
#include "usbh_device.h"
#include "usbh_desc.h"
#include "usbh_ch9.h"
#include "usbh_ch11.h"
#include "usbh_hub.h"

LOG_MODULE_REGISTER(usbh_hub, CONFIG_USBH_HUB_LOG_LEVEL);

K_MEM_SLAB_DEFINE_STATIC_TYPE(usbh_hub_slab, struct usbh_hub_data,
			       CONFIG_USBH_HUB_INSTANCES_COUNT);

static struct {
	uint8_t total_hubs;
	sys_slist_t hub_list;
	struct k_mutex lock;
} hub_mgr;

static int hub_interrupt_in_cb(struct usb_device *const udev,
			       struct uhc_transfer *const xfer);

static int hub_prepare_interrupt_xfer(struct usbh_hub_data *hub_data,
				      struct uhc_transfer **xfer_out)
{
	struct uhc_transfer *xfer;

	if (hub_data->int_ep == NULL) {
		LOG_ERR("No interrupt endpoint available");
		return -ENODEV;
	}

	xfer = usbh_xfer_alloc(hub_data->udev,
			       hub_data->int_ep->bEndpointAddress,
			       hub_interrupt_in_cb,
			       (void *)hub_data);
	if (xfer == NULL) {
		LOG_ERR("Failed to allocate interrupt transfer");
		return -ENOMEM;
	}

	*xfer_out = xfer;

	return 0;
}

static int hub_enqueue_interrupt(struct usbh_hub_data *hub_data,
				 struct uhc_transfer *xfer)
{
	struct net_buf *buf;
	int ret;

	buf = usbh_xfer_buf_alloc(hub_data->udev,
				  sys_le16_to_cpu(hub_data->int_ep->wMaxPacketSize));
	if (buf == NULL) {
		LOG_ERR("Failed to allocate interrupt buffer");
		usbh_xfer_free(hub_data->udev, xfer);
		k_mutex_lock(&hub_data->lock, K_FOREVER);
		hub_data->interrupt_transfer = NULL;
		k_mutex_unlock(&hub_data->lock);
		return -ENOMEM;
	}

	xfer->buf = buf;

	ret = usbh_xfer_enqueue(hub_data->udev, xfer);
	if (ret != 0) {
		LOG_ERR("Failed to enqueue interrupt transfer: %d", ret);
		usbh_xfer_buf_free(hub_data->udev, buf);
		usbh_xfer_free(hub_data->udev, xfer);
		k_mutex_lock(&hub_data->lock, K_FOREVER);
		hub_data->interrupt_transfer = NULL;
		k_mutex_unlock(&hub_data->lock);
		return ret;
	}

	return 0;
}

static int hub_start_interrupt(struct usbh_hub_data *hub_data)
{
	struct uhc_transfer *xfer;
	int ret;

	k_mutex_lock(&hub_data->lock, K_FOREVER);
	if (!hub_data->connected || hub_data->interrupt_transfer != NULL) {
		k_mutex_unlock(&hub_data->lock);
		return -EINVAL;
	}

	if (hub_data->state != HUB_STATE_OPERATIONAL) {
		k_mutex_unlock(&hub_data->lock);
		return -ENOENT;
	}
	k_mutex_unlock(&hub_data->lock);

	ret = hub_prepare_interrupt_xfer(hub_data, &xfer);
	if (ret != 0) {
		return ret;
	}

	k_mutex_lock(&hub_data->lock, K_FOREVER);
	hub_data->interrupt_transfer = xfer;
	k_mutex_unlock(&hub_data->lock);

	ret = hub_enqueue_interrupt(hub_data, xfer);
	if (ret != 0) {
		k_mutex_lock(&hub_data->lock, K_FOREVER);
		hub_data->interrupt_transfer = NULL;
		k_mutex_unlock(&hub_data->lock);
	}

	return ret;
}

static struct usbh_hub_data *const find_hub_by_udev(const struct usb_device *const udev)
{
	struct usbh_hub_data *hub_data;

	k_mutex_lock(&hub_mgr.lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER(&hub_mgr.hub_list, hub_data, node) {
		if (hub_data->udev == udev) {
			k_mutex_unlock(&hub_mgr.lock);
			return hub_data;
		}
	}

	k_mutex_unlock(&hub_mgr.lock);

	return NULL;
}

static void hub_process_data(struct usbh_hub_data *const hub_data)
{
	struct usb_hub_status *hub_sts;
	struct usb_hub_port *port;
	uint16_t hub_status;
	uint16_t hub_change;
	uint8_t scheduled_ports = 0;
	uint8_t port_index;
	bool hub_changed = false;
	bool restart = false;
	int ret;

	k_mutex_lock(&hub_data->lock, K_FOREVER);

	if (!hub_data->connected) {
		k_mutex_unlock(&hub_data->lock);
		return;
	}

	if (hub_data->state != HUB_STATE_OPERATIONAL) {
		LOG_WRN("Hub not ready for data processing (state=%d)",
			hub_data->state);
		k_mutex_unlock(&hub_data->lock);
		return;
	}

	for (port_index = 0; port_index <= hub_data->port_count; ++port_index) {
		if (((0x01U << (port_index & 0x07U)) &
		     (hub_data->int_buffer[port_index >> 3U])) == 0) {
			continue;
		}

		if (port_index != 0) {
			port = &hub_data->port_list[port_index - 1];
			LOG_DBG("Hub level %d port %d status changed, starting processing",
				hub_data->udev->level, port_index);
			hub_data->pending_ports++;
			scheduled_ports++;
			k_work_reschedule(&port->port_work, K_NO_WAIT);
		} else {
			/* hub-level change */
			hub_changed = true;
		}
	}

	if (scheduled_ports == 0 && !hub_changed &&
	    hub_data->interrupt_transfer == NULL &&
	    hub_data->connected) {
		restart = true;
	}

	k_mutex_unlock(&hub_data->lock);

	if (hub_changed) {
		hub_sts = &hub_data->status;

		LOG_INF("Hub level %d status changed, processing", hub_data->udev->level);
		ret = usbh_req_get_hub_status(hub_data->udev, &hub_status, &hub_change);
		if (ret != 0) {
			LOG_ERR("Failed to get hub status: %d", ret);
		} else {
			hub_sts->wHubStatus = hub_status;
			hub_sts->wHubChange = hub_change;

			LOG_DBG("Hub status: 0x%04x, change: 0x%04x",
				hub_sts->wHubStatus, hub_sts->wHubChange);

			if ((hub_sts->wHubChange & USB_HUB_CHANGE_LOCAL_POWER) != 0) {
				LOG_WRN("Hub local power status changed");
				ret = usbh_req_clear_hcfs_c_hub_local_power(hub_data->udev);
				if (ret != 0) {
					LOG_ERR("Failed to clear hub local power feature: %d",
						ret);
				}
			}

			if ((hub_sts->wHubChange & USB_HUB_CHANGE_OVER_CURRENT) != 0) {
				LOG_ERR("Hub over-current detected!");
				ret = usbh_req_clear_hcfs_c_hub_over_current(hub_data->udev);
				if (ret != 0) {
					LOG_ERR("Failed to clear hub over-current feature: %d",
						ret);
				}
			}
		}

		/* If no port work was scheduled, decide now whether to restart interrupt. */
		if (scheduled_ports == 0) {
			k_mutex_lock(&hub_data->lock, K_FOREVER);
			if (hub_data->interrupt_transfer == NULL && hub_data->connected) {
				restart = true;
			}
			k_mutex_unlock(&hub_data->lock);
		}
	}

	if (restart) {
		ret = hub_start_interrupt(hub_data);
		if (ret != 0) {
			LOG_ERR("Failed to start interrupt monitoring: %d", ret);
		}
	}
}

static int hub_interrupt_in_cb(struct usb_device *const udev,
			       struct uhc_transfer *const xfer)
{
	struct usbh_hub_data *const hub_data = (void *)xfer->priv;
	struct net_buf *buf = xfer->buf;
	int ret = 0;
	bool resubmit = false;

	k_mutex_lock(&hub_data->lock, K_FOREVER);

	if (!hub_data->connected) {
		hub_data->interrupt_transfer = NULL;
		k_mutex_unlock(&hub_data->lock);
		goto cleanup;
	}

	if (buf == NULL || buf->len == 0) {
		hub_data->interrupt_transfer = NULL;
		k_mutex_unlock(&hub_data->lock);
		LOG_ERR("Hub level %d interrupt transfer failed or no data",
			hub_data->udev->level);
		resubmit = true;
		goto cleanup;
	}

	memcpy(hub_data->int_buffer, buf->data,
	       MIN(buf->len, sizeof(hub_data->int_buffer)));

	LOG_DBG("Hub level %d interrupt data received: length=%d",
		hub_data->udev->level,
		buf->len);

	hub_data->interrupt_transfer = NULL;
	k_mutex_unlock(&hub_data->lock);

	k_work_submit(&hub_data->hub_work);
	goto cleanup;

cleanup:
	if (buf != NULL) {
		net_buf_unref(buf);
	}

	if (resubmit) {
		k_mutex_lock(&hub_data->lock, K_FOREVER);
		if (hub_data->connected && hub_data->state == HUB_STATE_OPERATIONAL) {
			k_mutex_unlock(&hub_data->lock);
			ret = hub_enqueue_interrupt(hub_data, xfer);
			if (ret != 0) {
				LOG_ERR("Failed to resubmit interrupt transfer: %d", ret);
			}
		} else {
			hub_data->interrupt_transfer = NULL;
			k_mutex_unlock(&hub_data->lock);
			usbh_xfer_free(hub_data->udev, xfer);
		}
	} else {
		usbh_xfer_free(hub_data->udev, xfer);
	}

	return 0;
}

static void hub_log_info(struct usbh_hub_data *const hub_data)
{
	const struct usb_device_descriptor *const dev_desc =
					&hub_data->udev->dev_desc;
	struct usb_device *udev = hub_data->udev;

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

static void hub_recursive_disconnect(struct usbh_hub_data *const hub_data)
{
	struct usb_device *port_udev;

	LOG_DBG("Recursively disconnecting Hub level %d and all children",
		hub_data->udev->level);

	k_work_cancel(&hub_data->hub_work);

	k_mutex_lock(&hub_data->lock, K_FOREVER);
	hub_data->pending_ports = 0;
	k_mutex_unlock(&hub_data->lock);

	for (uint8_t i = 0; i < hub_data->port_count; i++) {
		k_work_cancel_delayable(&hub_data->port_list[i].port_work);
		if (hub_data->port_list[i].udev != NULL) {
			port_udev = hub_data->port_list[i].udev;
			hub_data->port_list[i].udev = NULL;
			hub_data->port_list[i].state = PORT_STATE_NOT_CONFIGURED;
			usbh_device_disconnect(hub_data->uhs_ctx, port_udev);
		}
	}
}

static int enumerate_port_device(struct usbh_hub_data *hub_data,
				 struct usb_hub_port *port_instance,
				 uint8_t port_num,
				 struct usb_hub_port_status *port_sts)
{
	struct usb_device *udev;
	uint8_t speed;

	if ((port_sts->wPortStatus & USB_HUB_PORT_STATUS_HIGH_SPEED) != 0) {
		speed = USB_SPEED_SPEED_HS;
	} else if ((port_sts->wPortStatus & USB_HUB_PORT_STATUS_LOW_SPEED) != 0) {
		speed = USB_SPEED_SPEED_LS;
	} else {
		speed = USB_SPEED_SPEED_FS;
	}

	LOG_INF("Device ready on port %d (speed: %s)", port_num,
		speed == USB_SPEED_SPEED_HS ? "HIGH" :
		speed == USB_SPEED_SPEED_LS ? "LOW" : "FULL");

	udev = usbh_device_alloc(hub_data->uhs_ctx);
	if (udev == NULL) {
		LOG_ERR("Device enumeration failed for port %d", port_num);
		return -ENOMEM;
	}

	udev->hub_port = port_num;
	udev->speed = speed;
	udev->level = hub_data->udev->level + 1;
	udev->hub = hub_data->udev;

	usbh_device_connect(hub_data->uhs_ctx, udev);
	LOG_DBG("Device enumeration completed for port %d, addr=%d", port_num, udev->addr);

	port_instance->udev = udev;

	return 0;
}

static void hub_port_complete_processing(struct usbh_hub_data *hub_data,
					 struct usb_hub_port *port_instance,
					 uint8_t port_num)
{
	bool restart = false;
	int ret;

	if (port_instance != NULL &&
	    port_instance->state != PORT_STATE_DISABLED) {
		port_instance->reset_count = CONFIG_USBH_HUB_PORT_RESET_TIMES;
	}

	LOG_DBG("Port %d processing completed", port_num);

	k_mutex_lock(&hub_data->lock, K_FOREVER);
	if (hub_data->pending_ports > 0) {
		hub_data->pending_ports--;
		if (hub_data->pending_ports == 0 &&
		    hub_data->interrupt_transfer == NULL &&
		    hub_data->connected) {
			restart = true;
		}
	}
	k_mutex_unlock(&hub_data->lock);

	if (restart) {
		ret = hub_start_interrupt(hub_data);
		if (ret != 0) {
			LOG_ERR("Failed to restart interrupt monitoring: %d", ret);
		}
	}
}

static void hub_port_process(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct usb_hub_port *port_instance =
		CONTAINER_OF(dwork, struct usb_hub_port, port_work);
	struct usbh_hub_data *const hub_data = port_instance->hub;
	struct usb_hub_port_status port_sts;
	struct usb_device *udev;
	bool process_complete = false;
	uint16_t port_status;
	uint16_t port_change;
	uint8_t port_num;
	int ret;

	port_num = port_instance->num;

	k_mutex_lock(&hub_data->lock, K_FOREVER);

	if (!hub_data->connected) {
		if (hub_data->pending_ports > 0) {
			hub_data->pending_ports--;
		}
		k_mutex_unlock(&hub_data->lock);
		return;
	}

	k_mutex_unlock(&hub_data->lock);

	LOG_DBG("Processing port %d, state=%d", port_num, port_instance->state);

	switch (port_instance->state) {
	case PORT_STATE_POWERED_OFF:
		LOG_DBG("Port %d is powered off", port_num);
		process_complete = true;
		goto exit_processing;

	case PORT_STATE_NOT_CONFIGURED:
		LOG_ERR("Port %d in NOT_CONFIGURED state unexpectedly", port_num);
		process_complete = true;
		goto exit_processing;

	case PORT_STATE_DISCONNECTED: {
		LOG_DBG("Port %d: Getting port status", port_num);
		ret = usbh_req_get_port_status(hub_data->udev, port_num,
					       &port_status, &port_change);
		if (ret != 0) {
			LOG_ERR("Failed to get port status: %d", ret);
			goto error_recovery;
		}

		port_sts.wPortStatus = port_status;
		port_sts.wPortChange = port_change;
		LOG_DBG("Port %d status: wPortStatus=0x%04x, wPortChange=0x%04x", port_num,
			port_sts.wPortStatus, port_sts.wPortChange);

		if ((port_sts.wPortChange & USB_HUB_PORT_CHANGE_CONNECTION) != 0) {
			ret = usbh_req_clear_hcfs_c_port_connection(hub_data->udev,
								port_num);
			if (ret != 0) {
				LOG_ERR("Failed to clear port connection change: %d", ret);
				goto error_recovery;
			}
			LOG_DBG("Port %d: Cleared connection change bit", port_num);

			ret = usbh_req_get_port_status(hub_data->udev, port_num,
						       &port_status, &port_change);
			if (ret != 0) {
				LOG_ERR("Failed to get port connection status: %d", ret);
				goto error_recovery;
			}

			port_sts.wPortStatus = port_status;
			port_sts.wPortChange = port_change;

			if ((port_sts.wPortStatus & USB_HUB_PORT_STATUS_CONNECTION) == 0) {
				port_instance->state = PORT_STATE_DISCONNECTED;
				process_complete = true;
				goto exit_processing;
			}

			LOG_DBG("Port %d connection confirmed, moving to disabled then resetting",
				port_num);
			port_instance->state = PORT_STATE_DISABLED;
		} else if ((port_sts.wPortStatus & USB_HUB_PORT_STATUS_CONNECTION) != 0) {
			LOG_INF("Device connected to port %d, moving to disabled", port_num);
			port_instance->state = PORT_STATE_DISABLED;
		} else {
			if ((port_sts.wPortChange & USB_HUB_PORT_CHANGE_ENABLE) != 0) {
				ret = usbh_req_clear_hcfs_c_port_enable(
					hub_data->udev, port_num);
				if (ret != 0) {
					LOG_ERR("Failed to clear enable change: %d", ret);
				}
			}

			if ((port_sts.wPortChange & USB_HUB_PORT_CHANGE_SUSPEND) != 0) {
				LOG_DBG("Port %d suspend change detected", port_num);
				ret = usbh_req_clear_hcfs_c_port_suspend(
					hub_data->udev, port_num);
				if (ret != 0) {
					LOG_ERR("Failed to clear suspend change: %d", ret);
				}
			}

			if ((port_sts.wPortChange & USB_HUB_PORT_CHANGE_OVER_CURRENT) != 0) {
				LOG_WRN("Port %d over-current detected", port_num);
				ret = usbh_req_clear_hcfs_c_port_over_current(
					hub_data->udev, port_num);
				if (ret != 0) {
					LOG_ERR("Failed to clear over-current change: %d", ret);
				}
			}

			if ((port_sts.wPortChange & USB_HUB_PORT_CHANGE_RESET) != 0) {
				LOG_DBG("Port %d reset change detected", port_num);
				ret = usbh_req_clear_hcfs_c_port_reset(
					hub_data->udev, port_num);
				if (ret != 0) {
					LOG_ERR("Failed to clear reset change: %d", ret);
				}
			}

			process_complete = true;
			goto exit_processing;
		}

		if (port_instance->state != PORT_STATE_DISABLED) {
			process_complete = true;
			goto exit_processing;
		}
	}

	case PORT_STATE_DISABLED: {
		ret = usbh_req_get_port_status(hub_data->udev, port_num,
					       &port_status, &port_change);
		if (ret != 0) {
			LOG_ERR("Failed to get port status: %d", ret);
			goto error_recovery;
		}

		port_sts.wPortStatus = port_status;
		port_sts.wPortChange = port_change;

		if ((port_sts.wPortStatus & USB_HUB_PORT_STATUS_CONNECTION) == 0) {
			LOG_DBG("Port %d device disconnected during disabled state", port_num);
			port_instance->state = PORT_STATE_DISCONNECTED;
			goto process_disconnection;
		}

		if ((port_sts.wPortChange & USB_HUB_PORT_CHANGE_ENABLE) != 0) {
			ret = usbh_req_clear_hcfs_c_port_enable(hub_data->udev,
							    port_num);
			if (ret != 0) {
				LOG_ERR("Failed to clear enable change: %d", ret);
			}
		}

		if ((port_sts.wPortChange & USB_HUB_PORT_CHANGE_SUSPEND) != 0) {
			LOG_DBG("Port %d suspend change detected", port_num);
			ret = usbh_req_clear_hcfs_c_port_suspend(hub_data->udev,
							     port_num);
			if (ret != 0) {
				LOG_ERR("Failed to clear suspend change: %d", ret);
			}
		}

		if ((port_sts.wPortChange & USB_HUB_PORT_CHANGE_OVER_CURRENT) != 0) {
			LOG_WRN("Port %d over-current detected", port_num);
			ret = usbh_req_clear_hcfs_c_port_over_current(
				hub_data->udev, port_num);
			if (ret != 0) {
				LOG_ERR("Failed to clear over-current change: %d", ret);
			}
			process_complete = true;
			goto exit_processing;
		}

		if ((port_sts.wPortChange & USB_HUB_PORT_CHANGE_RESET) != 0) {
			LOG_DBG("Port %d reset change detected in disabled state", port_num);
			ret = usbh_req_clear_hcfs_c_port_reset(hub_data->udev,
							   port_num);
			if (ret != 0) {
				LOG_ERR("Failed to clear reset change: %d", ret);
			}
		}

		if (port_instance->reset_count == 0) {
			LOG_ERR("Port %d reset retry count exhausted, giving up", port_num);
			port_instance->state = PORT_STATE_DISABLED;
			process_complete = true;
			goto exit_processing;
		}

		ret = usbh_req_set_hcfs_port_reset(hub_data->udev, port_num);
		if (ret != 0) {
			LOG_ERR("Failed to reset port: %d", ret);
			goto error_recovery;
		}

		port_instance->reset_count--;

		port_instance->state = PORT_STATE_RESETTING;
		/* specification requires 10-20ms, Default is 20ms for the worst case scenario. */
		k_work_reschedule(&port_instance->port_work, K_MSEC(20));
		return;
	}

	case PORT_STATE_RESETTING: {
		ret = usbh_req_get_port_status(hub_data->udev, port_num,
					       &port_status, &port_change);
		if (ret != 0) {
			LOG_ERR("Failed to get port status for reset check: %d", ret);
			goto error_recovery;
		}

		port_sts.wPortStatus = port_status;
		port_sts.wPortChange = port_change;

		/* reset is not completed */
		if ((port_sts.wPortChange & USB_HUB_PORT_CHANGE_RESET) == 0) {
			if (port_instance->reset_count > 0) {
				LOG_WRN("Port %d reset timeout, retrying (%d left)",
					port_num, port_instance->reset_count);
				port_instance->state = PORT_STATE_DISABLED;
				k_work_reschedule(&port_instance->port_work,
					K_MSEC(CONFIG_USBH_HUB_ENUM_RETRY_DELAY_MS));
				return;
			}

			LOG_ERR("Port %d reset max retries exceeded", port_num);
			port_instance->state = PORT_STATE_DISABLED;
			process_complete = true;
			goto exit_processing;
		}

		LOG_DBG("Port %d reset completed", port_num);

		ret = usbh_req_clear_hcfs_c_port_reset(hub_data->udev, port_num);
		if (ret != 0) {
			LOG_ERR("Failed to clear reset feature: %d", ret);
			goto error_recovery;
		}
		LOG_DBG("Port %d: Cleared reset change bit", port_num);

		ret = usbh_req_get_port_status(hub_data->udev, port_num,
					       &port_status, &port_change);
		if (ret != 0) {
			LOG_ERR("Failed to get port status after reset: %d", ret);
			goto error_recovery;
		}

		port_sts.wPortStatus = port_status;
		port_sts.wPortChange = port_change;

		if ((port_sts.wPortStatus & USB_HUB_PORT_STATUS_CONNECTION) == 0) {
			LOG_WRN("Port %d device disconnected during reset", port_num);
			port_instance->state = PORT_STATE_DISCONNECTED;
			goto process_disconnection;
		}

		if ((port_sts.wPortStatus & USB_HUB_PORT_STATUS_ENABLE) == 0) {
			LOG_WRN("Port %d not enabled after reset", port_num);

			if (port_instance->reset_count > 0) {
				LOG_WRN("Port %d retrying reset (%d left)",
					port_num, port_instance->reset_count);
				port_instance->state = PORT_STATE_DISABLED;
				k_work_reschedule(&port_instance->port_work,
						K_MSEC(CONFIG_USBH_HUB_ENUM_RETRY_DELAY_MS));
				return;
			}

			LOG_ERR("Port %d max reset retries exceeded", port_num);
			port_instance->state = PORT_STATE_DISABLED;
			process_complete = true;
			goto exit_processing;
		}

		ret = enumerate_port_device(hub_data, port_instance, port_num, &port_sts);
		if (ret != 0) {
			if (port_instance->reset_count > 0) {
				LOG_WRN("Port %d enumeration failed, retrying reset (%d left)",
					port_num, port_instance->reset_count);
				port_instance->state = PORT_STATE_DISABLED;
				k_work_reschedule(&port_instance->port_work,
						K_MSEC(CONFIG_USBH_HUB_ENUM_RETRY_DELAY_MS));
				return;
			}

			LOG_ERR("Port %d enumeration max retries exceeded", port_num);
			port_instance->state = PORT_STATE_DISABLED;
			process_complete = true;
			goto exit_processing;
		}

		port_instance->state = PORT_STATE_ENABLED;
		process_complete = true;
		port_instance->reset_count = CONFIG_USBH_HUB_PORT_RESET_TIMES;
		goto exit_processing;
	}

	case PORT_STATE_ENABLED: {
		ret = usbh_req_get_port_status(hub_data->udev, port_num,
					       &port_status, &port_change);
		if (ret != 0) {
			LOG_ERR("Failed to get port status: %d", ret);
			goto error_recovery;
		}

		port_sts.wPortStatus = port_status;
		port_sts.wPortChange = port_change;

		if ((port_sts.wPortChange & USB_HUB_PORT_CHANGE_CONNECTION) != 0) {
			ret = usbh_req_clear_hcfs_c_port_connection(
				hub_data->udev, port_num);
			if (ret != 0) {
				LOG_ERR("Failed to clear connection change: %d", ret);
			}

			if ((port_sts.wPortStatus & USB_HUB_PORT_STATUS_CONNECTION) == 0) {
				port_instance->state = PORT_STATE_DISCONNECTED;
				goto process_disconnection;
			}
		}

		if ((port_sts.wPortChange & USB_HUB_PORT_CHANGE_ENABLE) != 0) {
			ret = usbh_req_clear_hcfs_c_port_enable(
				hub_data->udev, port_num);
			if (ret != 0) {
				LOG_ERR("Failed to clear enable change: %d", ret);
			}

			if ((port_sts.wPortStatus & USB_HUB_PORT_STATUS_ENABLE) == 0) {
				LOG_WRN("Port %d disabled due to error", port_num);
				port_instance->state = PORT_STATE_DISABLED;
			}
		}

		if ((port_sts.wPortChange & USB_HUB_PORT_CHANGE_SUSPEND) != 0) {
			LOG_DBG("Port %d suspend change detected", port_num);
			ret = usbh_req_clear_hcfs_c_port_suspend(
				hub_data->udev, port_num);
			if (ret != 0) {
				LOG_ERR("Failed to clear suspend change: %d", ret);
			}
		}

		if ((port_sts.wPortChange & USB_HUB_PORT_CHANGE_OVER_CURRENT) != 0) {
			LOG_WRN("Port %d over-current detected", port_num);
			ret = usbh_req_clear_hcfs_c_port_over_current(
				hub_data->udev, port_num);
			if (ret != 0) {
				LOG_ERR("Failed to clear over-current change: %d", ret);
			}
			port_instance->state = PORT_STATE_DISABLED;
		}

		if ((port_sts.wPortChange & USB_HUB_PORT_CHANGE_RESET) != 0) {
			LOG_DBG("Port %d reset change detected in enabled state", port_num);
			ret = usbh_req_clear_hcfs_c_port_reset(
				hub_data->udev, port_num);
			if (ret != 0) {
				LOG_ERR("Failed to clear reset change: %d", ret);
			}
		}

		process_complete = true;
		goto exit_processing;
	}

	default:
		LOG_ERR("Port %d in unsupported state: %d", port_num, port_instance->state);
		goto error_recovery;
	}

process_disconnection:
	if (port_instance->udev != NULL) {
		LOG_DBG("Device disconnected from Hub level %d port %d",
			hub_data->udev->level, port_num);

		udev = port_instance->udev;
		port_instance->udev = NULL;
		port_instance->state = PORT_STATE_DISCONNECTED;
		usbh_device_disconnect(hub_data->uhs_ctx, udev);
	} else {
		port_instance->state = PORT_STATE_DISCONNECTED;
	}
	process_complete = true;

exit_processing:
	if (process_complete) {
		hub_port_complete_processing(hub_data, port_instance, port_num);
	}
	return;

error_recovery:
	if (port_instance == NULL) {
		LOG_ERR("Port instance is NULL in error recovery");
		return;
	}

	if (port_instance->reset_count > 0) {
		port_instance->reset_count--;
		if (port_instance->udev != NULL) {
			udev = port_instance->udev;
			port_instance->udev = NULL;
			usbh_device_disconnect(hub_data->uhs_ctx, udev);
		}
		port_instance->state = PORT_STATE_DISCONNECTED;

		LOG_WRN("Port %d error recovery, %d retries left", port_num,
			port_instance->reset_count);
		k_work_reschedule(&port_instance->port_work,
				  K_MSEC(CONFIG_USBH_HUB_ERROR_RECOVERY_DELAY_MS));
		return;
	}

	LOG_ERR("Port %d max retries exceeded, moving to disabled", port_num);
	port_instance->state = PORT_STATE_DISABLED;
	process_complete = true;
	goto exit_processing;
}

static int hub_initialize(struct usbh_hub_data *hub_data)
{
	struct usbh_hub_data *parent_hub;
	struct usb_device *udev;
	uint16_t total_hub_desc_len = 0;
	int ret;

	LOG_DBG("Getting 7-byte hub descriptor");

	ret = usbh_req_desc_hub(hub_data->udev,
				hub_data->hub_desc_buf,
				sizeof(struct usb_hub_descriptor));
	if (ret != 0) {
		LOG_ERR("Failed to get hub descriptor: %d", ret);
		return ret;
	}

	udev = hub_data->udev;
	hub_data->port_count = hub_data->hub_desc.bNbrPorts;

	if (hub_data->port_count > ARRAY_SIZE(hub_data->port_list)) {
		LOG_WRN("Hub reports %d ports, clamping to %zu",
			hub_data->port_count, ARRAY_SIZE(hub_data->port_list));
		hub_data->port_count = ARRAY_SIZE(hub_data->port_list);
	}

	LOG_DBG("Hub has %d port_count", hub_data->port_count);

	/* Store hub think time */
	udev->tt = USB_HUB_GET_THINK_TIME(
		sys_le16_to_cpu(hub_data->hub_desc.wHubCharacteristics));
	LOG_DBG("hub think time: 0x%04x", udev->tt);

	total_hub_desc_len = 7 + ((hub_data->port_count + 7) >> 3) + 1;
	/* Get full hub descriptor */
	LOG_DBG("Getting full hub descriptor (length=%d)", total_hub_desc_len);
	ret = usbh_req_desc_hub(hub_data->udev,
				hub_data->hub_desc_buf,
				total_hub_desc_len);
	if (ret != 0) {
		LOG_ERR("Failed to get full hub descriptor: %d", ret);
		return ret;
	}

	for (uint8_t i = 0; i < hub_data->port_count; i++) {
		hub_data->port_list[i].udev = NULL;
		hub_data->port_list[i].hub = hub_data;
		hub_data->port_list[i].reset_count = CONFIG_USBH_HUB_PORT_RESET_TIMES;
		hub_data->port_list[i].state = PORT_STATE_POWERED_OFF;
		hub_data->port_list[i].num = i + 1;
		k_work_init_delayable(&hub_data->port_list[i].port_work, hub_port_process);
	}

	for (uint8_t i = 0; i < hub_data->port_count; i++) {
		LOG_DBG("Setting port %d power", i + 1);
		ret = usbh_req_set_hcfs_port_power(hub_data->udev, i + 1);
		if (ret != 0) {
			LOG_ERR("Failed to set port %d power: %d", i + 1, ret);
		} else {
			hub_data->port_list[i].state = PORT_STATE_DISCONNECTED;
		}
	}

	/* this hub has a parent hub, add to parent's child list */
	if (hub_data->udev->hub != NULL) {
		parent_hub = find_hub_by_udev(hub_data->udev->hub);
		if (parent_hub != NULL) {
			sys_slist_append(&parent_hub->child_hubs, &hub_data->child_node);
		}
	}

	hub_log_info(hub_data);

	return 0;
}

static void hub_process(struct k_work *work)
{
	struct usbh_hub_data *hub_data =
		CONTAINER_OF(work, struct usbh_hub_data, hub_work);
	int ret;

	k_mutex_lock(&hub_data->lock, K_FOREVER);

	if (!hub_data->connected) {
		k_mutex_unlock(&hub_data->lock);
		return;
	}

	if (hub_data->state == HUB_STATE_OPERATIONAL) {
		k_mutex_unlock(&hub_data->lock);
		hub_process_data(hub_data);
		return;
	}

	if (hub_data->state != HUB_STATE_INIT) {
		LOG_WRN("Hub not in INIT state, skipping initialization");
		k_mutex_unlock(&hub_data->lock);
		return;
	}

	k_mutex_unlock(&hub_data->lock);

	ret = hub_initialize(hub_data);

	k_mutex_lock(&hub_data->lock, K_FOREVER);

	/* hub may have been removed while hub_initialize */
	if (!hub_data->connected) {
		k_mutex_unlock(&hub_data->lock);
		return;
	}

	if (ret != 0) {
		hub_data->state = HUB_STATE_ERROR;
		k_mutex_unlock(&hub_data->lock);
		LOG_ERR("Hub initialization failed: %d", ret);
		return;
	}

	hub_data->state = HUB_STATE_OPERATIONAL;
	k_mutex_unlock(&hub_data->lock);

	if (hub_data->interrupt_transfer == NULL && hub_data->connected) {
		ret = hub_start_interrupt(hub_data);
		if (ret != 0) {
			LOG_ERR("Failed to start interrupt monitoring: %d", ret);
		}
	}
}

static int usbh_hub_probe(struct usbh_class_data *const c_data,
			  struct usb_device *const udev,
			  const uint8_t iface)
{
	struct usbh_hub_data *hub_data;
	const struct usb_desc_header *header;
	const void *desc_start;
	const void *desc_end;
	uint8_t target_iface;

	if (hub_mgr.total_hubs == CONFIG_USBH_HUB_INSTANCES_COUNT) {
		LOG_ERR("Maximum number of hubs reached (%d)", CONFIG_USBH_HUB_INSTANCES_COUNT);
		return -ENOTSUP;
	}

	if (udev->level > CONFIG_USBH_HUB_MAX_LEVELS) {
		LOG_ERR("Hub chain depth limit exceeded (%d > %d)",
			udev->level,
			CONFIG_USBH_HUB_MAX_LEVELS);
		return -ENOSPC;
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

	if (k_mem_slab_alloc(&usbh_hub_slab, (void **)&hub_data, K_NO_WAIT) != 0) {
		LOG_ERR("Failed to allocate HUB management data");
		return -ENOTSUP;
	}

	memset(hub_data, 0, sizeof(*hub_data));

	hub_data->udev = udev;
	hub_data->uhs_ctx = (struct usbh_context *)udev->ctx;
	hub_data->state = HUB_STATE_INIT;

	sys_slist_init(&hub_data->child_hubs);

	/* Parse interrupt endpoint within the interface descriptors */
	header = (const void *)desc_start;
	while (header != NULL) {
		/* Stop if we've reached the next function */
		if ((desc_end != NULL) && ((void *)header >= desc_end)) {
			break;
		}

		if (usbh_desc_is_valid_endpoint(header)) {
			const struct usb_ep_descriptor *ep_desc = (const void *)header;

			if ((ep_desc->bEndpointAddress & USB_EP_DIR_MASK) == USB_EP_DIR_IN &&
			    (ep_desc->bmAttributes & USB_EP_TRANSFER_TYPE_MASK) ==
				    USB_EP_TYPE_INTERRUPT) {
				hub_data->int_ep = ep_desc;
				LOG_DBG("Found hub interrupt IN endpoint 0x%02x",
					ep_desc->bEndpointAddress);
				break;
			}
		}

		header = usbh_desc_get_next(header);
	}

	hub_data->connected = true;
	hub_data->interrupt_transfer = NULL;

	k_mutex_init(&hub_data->lock);
	k_work_init(&hub_data->hub_work, hub_process);

	c_data->priv = hub_data;

	k_mutex_lock(&hub_mgr.lock, K_FOREVER);
	sys_slist_append(&hub_mgr.hub_list, &hub_data->node);
	hub_mgr.total_hubs++;
	k_mutex_unlock(&hub_mgr.lock);

	k_work_submit(&hub_data->hub_work);

	return 0;
}

static int usbh_hub_removed(struct usbh_class_data *const cdata)
{
	struct usbh_hub_data *hub_data;
	uint16_t vendor_id;
	uint16_t product_id;
	uint8_t level;
	int ret;

	hub_data = cdata->priv;

	level = hub_data->udev->level;
	vendor_id = sys_le16_to_cpu(hub_data->udev->dev_desc.idVendor);
	product_id = sys_le16_to_cpu(hub_data->udev->dev_desc.idProduct);

	k_mutex_lock(&hub_data->lock, K_FOREVER);
	hub_data->connected = false;
	hub_data->pending_ports = 0;
	k_mutex_unlock(&hub_data->lock);

	/* Recursively disconnect all child hubs and devices */
	hub_recursive_disconnect(hub_data);

	k_mutex_lock(&hub_data->lock, K_FOREVER);
	if (hub_data->interrupt_transfer != NULL) {
		ret = usbh_xfer_dequeue(hub_data->udev,
					hub_data->interrupt_transfer);
		if (ret != 0) {
			LOG_ERR("Failed to dequeue interrupt transfer: %d", ret);
		}

		hub_data->interrupt_transfer = NULL;

		LOG_DBG("Interrupt transfer cancelled");
	}

	if (hub_data->udev->hub != NULL) {
		struct usbh_hub_data *parent_hub;

		parent_hub = find_hub_by_udev(hub_data->udev->hub);
		if (parent_hub != NULL) {
			sys_slist_find_and_remove(&parent_hub->child_hubs,
							&hub_data->child_node);
		}
	}
	k_mutex_unlock(&hub_data->lock);

	k_mutex_lock(&hub_mgr.lock, K_FOREVER);
	sys_slist_find_and_remove(&hub_mgr.hub_list, &hub_data->node);
	if (hub_mgr.total_hubs > 0) {
		hub_mgr.total_hubs--;
	}
	k_mutex_unlock(&hub_mgr.lock);

	LOG_INF("Hub (level %d, Vendor ID: 0x%04x, Product ID: 0x%04x) removal completed",
		level, vendor_id, product_id);

	k_mem_slab_free(&usbh_hub_slab, (void *)hub_data);
	cdata->priv = NULL;

	return 0;
}

static int usbh_hub_init(struct usbh_class_data *const c_data)
{
	return 0;
}

static int usbh_hub_pre_init(void)
{
	sys_slist_init(&hub_mgr.hub_list);
	k_mutex_init(&hub_mgr.lock);
	hub_mgr.total_hubs = 0;

	return 0;
}

static struct usbh_class_filter hub_filters[] = {
	{
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
		.class = USB_HUB_CLASS_CODE,
		.sub = USB_HUB_SUBCLASS_CODE,
		.proto = USB_HUB_PROTOCOL_CODE,
	},
	{0},
};

static struct usbh_class_api usbh_hub_class_api = {
	.init = usbh_hub_init,
	.probe = usbh_hub_probe,
	.removed = usbh_hub_removed,
};

SYS_INIT(usbh_hub_pre_init, POST_KERNEL, CONFIG_USBH_HUB_INIT_PRIO);

#define USBH_DEFINE_HUB_CLASS(i, _)                                                                \
	USBH_DEFINE_CLASS(UTIL_CAT(usbh_hub_class_, i), &usbh_hub_class_api, NULL, hub_filters)

LISTIFY(CONFIG_USBH_HUB_INSTANCES_COUNT, USBH_DEFINE_HUB_CLASS, (;), _)
