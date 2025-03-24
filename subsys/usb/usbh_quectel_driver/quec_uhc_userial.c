/*
 * Copyright (c) 2025 Quectel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <soc.h>
#include <string.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include "quec_uhc_driver.h"

LOG_MODULE_REGISTER(quec_uhc_serial, LOG_LEVEL_ERR);
/*===========================================================================
 * 							  variables
 ===========================================================================*/
static quec_uhc_mgr_t udv_manager;

static void quec_uhc_event_hub(uint32_t event, void *ctx);
/*===========================================================================
 * 							  functions
 ===========================================================================*/
static int quec_uhc_disconnect_irq(quec_uhc_mgr_t *udev)
{	
	udev->status = QUEC_STATUS_DISCONNECT;
	quec_uhc_sio_deinit(udev);

	quec_uhc_msg_put(udev->sys_port.msgq, QUEC_DEVICE_DISCONNECT, 0, 0);
	return 0;
}

static void quec_uhc_event_hub(uint32_t event, void *ctx)
{
	quec_uhc_pmg_t *port = &udv_manager.sys_port;

	switch(event)
	{
		case QUEC_DEVICE_CONNECT:
			quec_print("connect irq");
			quec_uhc_msg_put(port->msgq, QUEC_DEVICE_CONNECT, 0, 0);
		break;

		case QUEC_DEVICE_DISCONNECT:
			quec_print("dsiconnect irq");
			quec_uhc_disconnect_irq(&udv_manager);
		break;

		default:

		break;
	}
}

static void quec_uhc_tx_callback(void *ctx)
{
	int ret = 0;
	quec_uhc_xfer_t *xfer = ctx;
	quec_trans_status_t t_event;
	uint32_t tx_remain, tx_size;

	if(xfer == NULL || xfer->status != 0)
	{
		quec_print("transfer err %p", xfer);
		return;
	}

	quec_uhc_pmg_t *port = &udv_manager.dev[xfer->cdc_num].tx_port;
	if(port->is_busy != true)
	{
		quec_print("port %d tx error %d", xfer->cdc_num, port->is_busy);
		
		port->is_busy = false;
		t_event.status = QUEC_TX_ERROR;
		k_msgq_put(port->msgq, &t_event, K_NO_WAIT);
		return;	
	}

	tx_remain = ring_buffer_num_items(&port->fifo);
	if(tx_remain == 0)
	{
		port->is_busy = false;

		t_event.status = QUEC_TX_COMPLETE;
		t_event.cdc_num = xfer->port_num;
		t_event.size = xfer->actual;
		k_msgq_put(port->msgq, &t_event, K_NO_WAIT);
	}
	else
	{
		tx_size = ring_buffer_num_items(&port->fifo);
		tx_size = UHC_MIN(64, tx_size);
		xfer->nbytes = tx_size;
		ring_buffer_read(&port->fifo, port->cache, tx_size);	
		
		ret = udv_manager.api->enqueue(udv_manager.device, xfer);
		if(ret < 0)
		{
			quec_print("tx fail port %d", xfer->cdc_num);
			port->is_busy = false;
			return;
		}
	}
}

static void quec_uhc_rx_callback(void *ctx)
{
	int ret = 0;
	uint32_t rx_size, free_szie = 0;
	quec_uhc_xfer_t *xfer = ctx;
	quec_trans_status_t t_event = {0, 0, 0};
	quec_uhc_pmg_t *port = &udv_manager.dev[xfer->cdc_num].rx_port;

	if(xfer == NULL)
	{
		quec_print("rx param error");
		return;
	}

	if(xfer->status != 0 || port->is_busy == false)
	{
		port->is_busy = false; //停止传输
		t_event.status = QUEC_RX_ERROR;
		k_msgq_put(port->msgq, &t_event, K_NO_WAIT);
		quec_print("rx status error %d %d", xfer->status, port->is_busy);
		return;
	}

	free_szie = ring_buffer_free_size(&port->fifo);
	if(free_szie < xfer->actual)
	{
		quec_print("port %d rx overflow %d %d", xfer->cdc_num, free_szie, xfer->actual);
	}

	rx_size = UHC_MIN(free_szie, xfer->actual);
	if(rx_size > 0)
	{
		xfer->cached += rx_size;
		ring_buffer_write(port->cache, rx_size, &port->fifo);
		quec_print("rx: %d", rx_size);
	}

	free_szie = ring_buffer_free_size(&port->fifo);
	if(free_szie < USB_FS_PKT_SIZE)
	{
		quec_print("port %d rx full", xfer->cdc_num);
		port->is_busy = false; //停止传输
	}
	else
	{
		xfer->nbytes = USB_FS_PKT_SIZE;
		port->is_busy = true;
		
		ret = udv_manager.api->enqueue(udv_manager.device, xfer);
		if(ret < 0)
		{
			quec_print("rx fail port %d", xfer->cdc_num);
			port->is_busy = false;
			return;
		}
	}

	if((xfer->actual < USB_FS_PKT_SIZE && xfer->actual > 0) ||\
		(xfer->cached > 0 && xfer->actual == 0) ||\
		(xfer->cached >= USB_RX_TRIG_LEVEL) ||\
		(port->is_busy == false))
	{
		t_event.size = xfer->cached;
		t_event.cdc_num = xfer->cdc_num;
		t_event.status |= QUEC_RX_ARRIVE;
		xfer->cached = 0;
		k_msgq_put(port->msgq, &t_event, K_NO_WAIT);		
	}
}

static int quec_uhc_connect_handler(quec_uhc_mgr_t *udev)
{
	usb_device_desc_t device_desc;
	uhc_cfg_descriptor_t config_desc;
	usb_intf_ep_desc_t intf_ep_desc;

	if(quec_uhc_enum_process(udev, &device_desc, &config_desc) < 0)
	{
		quec_print("enumeration failed");
		return -1;
	}

	if(quec_uhc_parse_config_desc(&config_desc, QUEC_AT_INTF_NUM, &intf_ep_desc) == 0)
	{
		quec_uhc_dev_t *at_port = &udev->dev[QUEC_AT_PORT];

		memcpy(&at_port->rx_port.ep_desc, &intf_ep_desc.in_ep_desc, sizeof(usb_endp_desc_t));
		memcpy(&at_port->tx_port.ep_desc, &intf_ep_desc.out_ep_desc, sizeof(usb_endp_desc_t));
		memcpy(&at_port->ctl_port.ep_desc, &intf_ep_desc.ctrl_ep_desc, sizeof(usb_endp_desc_t));

		at_port->status = QUEC_PORT_STATUS_FREE;
	}

	if(quec_uhc_parse_config_desc(&config_desc, QUEC_MODEM_INTF_NUM, &intf_ep_desc) == 0)
	{
		quec_uhc_dev_t *modem_port = &udev->dev[QUEC_MODEM_PORT];

		memcpy(&modem_port->rx_port.ep_desc, &intf_ep_desc.in_ep_desc, sizeof(usb_endp_desc_t));
		memcpy(&modem_port->tx_port.ep_desc, &intf_ep_desc.out_ep_desc, sizeof(usb_endp_desc_t));
		memcpy(&modem_port->ctl_port.ep_desc, &intf_ep_desc.ctrl_ep_desc, sizeof(usb_endp_desc_t));

		modem_port->status = QUEC_PORT_STATUS_FREE;
	}

	udev->status = QUEC_STATUS_CONNECT;
	if(udev->user_callback)
	{
		udev->user_callback(QUEC_DEVICE_CONNECT, 0, 0);
	}

	return 0;
}

static int quec_uhc_disconnect_handler(quec_uhc_mgr_t *udev)
{
	uint32_t irq_hd = irq_lock();
	
	if(udev->dev[QUEC_AT_PORT].rx_port.port_num > 0)
	{
		udev->api->ep_disable(udev->device, udev->dev[QUEC_AT_PORT].rx_port.port_num);
		udev->api->ep_disable(udev->device, udev->dev[QUEC_AT_PORT].tx_port.port_num);
		udev->dev[QUEC_AT_PORT].rx_port.port_num = -1;
		udev->dev[QUEC_AT_PORT].tx_port.port_num = -1;
	}

	if(udev->dev[QUEC_MODEM_PORT].rx_port.port_num > 0)
	{
		udev->api->ep_disable(udev->device, udev->dev[QUEC_MODEM_PORT].rx_port.port_num);
		udev->api->ep_disable(udev->device, udev->dev[QUEC_MODEM_PORT].tx_port.port_num);
		udev->dev[QUEC_MODEM_PORT].rx_port.port_num = -1;
		udev->dev[QUEC_MODEM_PORT].tx_port.port_num = -1;
	}

	udev->api->deinit(udev->device);
	udev->api->init(udev->device, quec_uhc_event_hub);

	irq_unlock(irq_hd);

	if(udev->user_callback)
	{
		udev->user_callback(QUEC_DEVICE_DISCONNECT, 0, 0);
	}

	return 0;
}

static void quec_uhc_sys_process(void *ctx1, void *ctx2, void *ctx3)
{
	int ret = 0;
	quec_uhc_msg_t uhc_msg;
	quec_uhc_pmg_t *sys_port = (quec_uhc_pmg_t *)ctx1;
	quec_uhc_mgr_t *udev = (quec_uhc_mgr_t *)ctx2;

	while(1)
	{
		ret = k_msgq_get(sys_port->msgq, &uhc_msg, K_FOREVER);
		if(ret != 0)
		{
			quec_print("message error");
			continue;
		}
		
		switch(uhc_msg.event_id)
		{
			case QUEC_DEVICE_CONNECT:
				quec_uhc_connect_handler(udev);
			break;

			case QUEC_DEVICE_DISCONNECT:
				quec_uhc_disconnect_handler(udev);
			break;
			
			default:

			break;
		}
	}
}

static void quec_uhc_rx_process(void *ctx1, void *ctx2, void *ctx3)
{
	int ret = 0, total_size = 0;
	quec_trans_status_t r_event;
	quec_uhc_pmg_t *port = (quec_uhc_pmg_t *)ctx1;
	quec_uhc_mgr_t *udev = (quec_uhc_mgr_t *)ctx2;

	while(1)
	{	
		ret = k_msgq_get(port->msgq, &r_event, K_FOREVER);
		if(ret != 0)
		{
			quec_print("message error");
			continue;
		}

		if(r_event.status & QUEC_RX_ARRIVE)
		{			
			uint32_t irq_hd = irq_lock();
			total_size = ring_buffer_num_items(&port->fifo);
			irq_unlock(irq_hd);

			if(udev->user_callback && total_size > 0)
			{
				udev->user_callback(r_event.status, r_event.cdc_num, total_size);
			}
		}
		else
		{
			if(udev->user_callback)
			{
				udev->user_callback(r_event.status, r_event.cdc_num, 0);
			}
		}
	}
}

static void quec_uhc_tx_process(void *ctx1, void *ctx2, void *ctx3)
{
	int ret = 0;
	quec_trans_status_t t_event;
	quec_uhc_pmg_t *port = (quec_uhc_pmg_t *)ctx1;
	quec_uhc_mgr_t *udev = (quec_uhc_mgr_t *)ctx2;

	while(1)
	{
		ret = k_msgq_get(port->msgq, &t_event, K_FOREVER);
		if(ret != 0)
		{
			quec_print("message error");
			continue;
		}

		if(udev->user_callback)
		{
			udev->user_callback(t_event.status, t_event.cdc_num, t_event.size);
		}
	}
}

static int quec_uhc_cdc_init(quec_uhc_mgr_t *cdc, uint8_t port)
{
	struct k_thread *res = NULL;

	if(port == QUEC_SYSTEM_PORT)
	{
		quec_uhc_sys_memory_init(&cdc->sys_port);

		res = k_thread_create(&cdc->sys_port.thread,
								(k_thread_stack_t *)cdc->sys_port.task_stack,
								QUEC_RX_STACK_SIZE, quec_uhc_sys_process,
								(void *)&cdc->sys_port,
								(void *)cdc, NULL,
								K_PRIO_PREEMPT(5), 0, K_MSEC(0));
		
		if(res == NULL)
		{
			quec_print("rx thread init fail");
			return -1;
		}		
	}
	else
	{
		quec_uhc_dev_t *dev = &cdc->dev[port];
	
		dev->intf_num = (port == QUEC_AT_PORT ? QUEC_AT_INTF_NUM : QUEC_MODEM_INTF_NUM);
		quec_uhc_cdc_memory_init(dev, port);

		res = k_thread_create(&dev->rx_port.thread,
								(k_thread_stack_t *)dev->rx_port.task_stack,
								QUEC_RX_STACK_SIZE, quec_uhc_rx_process,
								(void *)&dev->rx_port,
								(void *)&udv_manager, NULL,
								K_PRIO_PREEMPT(5), 0, K_MSEC(0));
		
		if(res == NULL)
		{
			quec_print("rx thread init fail");
			return -1;
		}

		res = k_thread_create(&dev->tx_port.thread,
								(k_thread_stack_t *)dev->tx_port.task_stack,
								QUEC_RX_STACK_SIZE, quec_uhc_tx_process,
								(void *)&dev->tx_port,
								(void *)&udv_manager, NULL,
								K_PRIO_PREEMPT(5), 0, K_MSEC(0));
		if(res == NULL)
		{
			quec_print("tx thread init fail");
			return -1;
		}
	}

	return 0;
}

static int quec_uhc_start(const struct device *dev)
{
	quec_uhc_mgr_t *dev_cfg = (quec_uhc_mgr_t *)dev->data;
	if(dev_cfg == NULL)
	{
		quec_print("device data error");
		return -1;	
	}	

	const struct device *cdc_dev = device_get_binding("QCX216");	
	if(cdc_dev == NULL)
	{
		quec_print("no invalid device");
		return -1;
	}
	
	quec_udrv_api_t *api = (quec_udrv_api_t *)cdc_dev->api;
	if(api == NULL)
	{
		quec_print("device callback error");
		return -1;	
	}

	dev_cfg->device = cdc_dev;
	dev_cfg->api = api;
	dev_cfg->dev_address = 0;

	quec_uhc_cdc_init(dev_cfg, QUEC_SYSTEM_PORT);
	quec_uhc_cdc_init(dev_cfg, QUEC_AT_PORT);
	quec_uhc_cdc_init(dev_cfg, QUEC_MODEM_PORT);

	api->set_address(cdc_dev, 0);
	api->init(cdc_dev, quec_uhc_event_hub);

	quec_print("uhc init done");
	return 0;
}

int quec_uhc_sio_deinit(quec_uhc_mgr_t *udev)
{
	uint8_t port_id = QUEC_AT_PORT;
	quec_trans_status_t t_event = {0};

	for(; port_id < QUEC_PORT_MAX; port_id++)
	{
		quec_uhc_dev_t *cdc_dev = (quec_uhc_dev_t *)&udev->dev[port_id];

		cdc_dev->status = QUEC_PORT_STATUS_INVALID;
		
		ring_buffer_reset(&cdc_dev->rx_port.fifo);
		ring_buffer_reset(&cdc_dev->tx_port.fifo);
		memset(&cdc_dev->rx_port.xfer, 0, sizeof(quec_uhc_xfer_t));
		memset(&cdc_dev->tx_port.xfer, 0, sizeof(quec_uhc_xfer_t));

		if(cdc_dev->rx_port.is_busy)
		{
			cdc_dev->rx_port.is_busy = false;
			t_event.status = QUEC_RX_ERROR;
			k_msgq_put(cdc_dev->rx_port.msgq, &t_event, K_NO_WAIT);
		}

		if(cdc_dev->tx_port.is_busy)
		{
			cdc_dev->tx_port.is_busy = false;
			t_event.status = QUEC_TX_ERROR;
			k_msgq_put(cdc_dev->tx_port.msgq, &t_event, K_NO_WAIT);
		}
	}

	return 0;
}

int quec_uhc_msg_put(struct k_msgq *msgq, uint32_t event_id, uint32_t param1, uint32_t param2)
{
	quec_uhc_msg_t uhc_msg = {0};

	uhc_msg.event_id = event_id;
	uhc_msg.param1 = param1;
	uhc_msg.param2 = param2;

	k_msgq_put(msgq, &uhc_msg, K_NO_WAIT);
	return 0;
}

int quec_uhc_open(const struct device *dev, quec_cdc_port_e port_id)
{
	quec_uhc_mgr_t *udev = (quec_uhc_mgr_t *)dev->data;
	
	if(udev == NULL || (port_id != QUEC_AT_PORT && port_id != QUEC_MODEM_PORT))
	{
		return -1;
	}

	quec_uhc_dev_t *uhc_port = &udev->dev[port_id];
	if(uhc_port->status != QUEC_PORT_STATUS_FREE)
	{
		quec_print("port %d status err %d %p", port_id, uhc_port->status, uhc_port);
		return -1;
	}

	if(quec_uhc_set_interface(udev, uhc_port->intf_num))
	{
		quec_print("set interface err");
		return -1;
	}

	if(quec_uhc_set_line_state(udev, uhc_port->intf_num, true))
	{
		quec_print("set line_state err");
		return -1;
	}

	uint32_t irq_hd = irq_lock();

	uhc_port->status = QUEC_PORT_STATUS_OPEN;
	uhc_port->rx_port.port_num = udev->api->ep_enable(udev->device, &uhc_port->rx_port.ep_desc);
	uhc_port->tx_port.port_num = udev->api->ep_enable(udev->device, &uhc_port->tx_port.ep_desc);
	uhc_port->rx_port.is_busy = true;
	
	quec_uhc_xfer_t *xfer = &uhc_port->rx_port.xfer;
	memset(xfer, 0, sizeof(quec_uhc_xfer_t));			
	xfer->ep_desc = &uhc_port->rx_port.ep_desc;
	xfer->port_num = uhc_port->rx_port.port_num;
	xfer->cdc_num = port_id;
	xfer->buffer = uhc_port->rx_port.cache;
	xfer->trans_id = udev->trans;
	xfer->token = USBH_PID_DATA;
	xfer->nbytes = USB_FS_PKT_SIZE;
	xfer->callback = quec_uhc_rx_callback;

	int ret = udev->api->enqueue(udev->device, xfer);
	if(ret < 0)
	{
		quec_print("port %d rx start fail", port_id);
		uhc_port->rx_port.is_busy = false;
		irq_unlock(irq_hd);
		return -1;
	}

	irq_unlock(irq_hd);	
	
	quec_print("port %d open done rx %d tx %d", port_id, uhc_port->rx_port.port_num, uhc_port->tx_port.port_num);
	return 0;
}

int quec_uhc_close(const struct device *dev, quec_cdc_port_e port_id)
{
	quec_uhc_mgr_t *udev = (quec_uhc_mgr_t *)dev->data;
	
	if(udev == NULL || (port_id != QUEC_AT_PORT && port_id != QUEC_MODEM_PORT))
	{
		return -1;
	}

	quec_uhc_dev_t *uhc_port = &udev->dev[port_id];
	if(uhc_port->status != QUEC_PORT_STATUS_OPEN)
	{
		quec_print("port %d status err %d %p", port_id, uhc_port->status, uhc_port);
		return -1;
	}

	if(quec_uhc_set_line_state(udev, uhc_port->intf_num, false))
	{
		quec_print("set line_state err");
		return -1;
	}

	uint32_t irq_hd = irq_lock();

	uhc_port->status = QUEC_PORT_STATUS_FREE;
	udev->api->ep_disable(udev->device, uhc_port->rx_port.port_num);
	udev->api->ep_disable(udev->device, uhc_port->tx_port.port_num);
	uhc_port->rx_port.port_num = -1;
	uhc_port->tx_port.port_num = -1;

	ring_buffer_reset(&uhc_port->rx_port.fifo);
	ring_buffer_reset(&uhc_port->tx_port.fifo);
	memset(&uhc_port->rx_port.xfer, 0, sizeof(quec_uhc_xfer_t));
	memset(&uhc_port->tx_port.xfer, 0, sizeof(quec_uhc_xfer_t));
	uhc_port->rx_port.is_busy = false;
	uhc_port->tx_port.is_busy = false;

	irq_unlock(irq_hd);
	
	quec_print("port %d close done", port_id);
	return 0;
}

int quec_uhc_read(const struct device *dev, quec_cdc_port_e port_id, uint8_t *buffer, uint32_t size)
{
	quec_uhc_mgr_t *udev = (quec_uhc_mgr_t *)dev->data;
	int rec_size = 0, free_size = 0;
	
	if(udev == NULL || (port_id != QUEC_AT_PORT && port_id != QUEC_MODEM_PORT))
	{
		quec_print("param err %p %d", udev, port_id);
		return -1;
	}

	uint32_t irq_hd = irq_lock();
	
	quec_uhc_dev_t *uhc_port = &udev->dev[port_id];
	quec_uhc_pmg_t *port = &uhc_port->rx_port;
	if(uhc_port->status != QUEC_PORT_STATUS_OPEN)
	{
		quec_print("port %d status err %d %p", port_id, uhc_port->status, uhc_port);
		irq_unlock(irq_hd);
		return -1;
	}

	rec_size = ring_buffer_num_items(&port->fifo);
	if(rec_size > 0)
	{
		rec_size = UHC_MIN(size, rec_size);
		ring_buffer_read(&port->fifo, buffer, rec_size);
	}

	if(uhc_port->rx_port.is_busy == false)
	{
		free_size = ring_buffer_free_size(&port->fifo);
		if(free_size >= USB_FS_PKT_SIZE)
		{
			quec_uhc_xfer_t *xfer = &port->xfer;

			xfer->ep_desc = &port->ep_desc;
			xfer->port_num = port->port_num;
			xfer->cdc_num = port_id;
			xfer->buffer = port->cache;
			xfer->trans_id = udev->trans;
			xfer->token = USBH_PID_DATA;
			xfer->nbytes = USB_FS_PKT_SIZE;
			xfer->callback = quec_uhc_rx_callback;			
			port->is_busy = true;

			int ret = udev->api->enqueue(udev->device, &port->xfer);
			if(ret < 0)
			{
				quec_print("rx port %d start fail", xfer->cdc_num);
				port->is_busy = false;
				irq_unlock(irq_hd);
				return -1;
			}
		}
	}
	
	irq_unlock(irq_hd);
	return rec_size;
}

int quec_uhc_write(const struct device *dev, quec_cdc_port_e port_id, uint8_t *buffer, uint32_t size)
{
	quec_uhc_mgr_t *udev = (quec_uhc_mgr_t *)dev->data;
	int tx_size = 0, wr_size = 0, ret = 0;
	uint32_t irq_hd = 0;
	
	if(udev == NULL || (port_id != QUEC_AT_PORT && port_id != QUEC_MODEM_PORT))
	{
		return -1;
	}

	irq_hd = irq_lock();
	
	quec_uhc_dev_t *uhc_port = &udev->dev[port_id];
	quec_uhc_pmg_t *port = &uhc_port->tx_port;
	if(uhc_port->status != QUEC_PORT_STATUS_OPEN)
	{
		quec_print("port %d not open err %d %p", port_id, uhc_port->status, uhc_port);
		irq_unlock(irq_hd);
		return -1;
		
	}
	tx_size = ring_buffer_free_size(&port->fifo);
	if(tx_size > 0)
	{
		tx_size = UHC_MIN(size, tx_size);
		ring_buffer_write(buffer, tx_size, &port->fifo);

		if(!port->is_busy)
		{	
			port->is_busy = true;
			wr_size = ring_buffer_num_items(&port->fifo);
			wr_size = UHC_MIN(64, wr_size);
			ring_buffer_read(&port->fifo, port->cache, wr_size);
		
			quec_uhc_xfer_t *xfer = &port->xfer;	
			memset(xfer, 0, sizeof(quec_uhc_xfer_t));			
			xfer->ep_desc = &port->ep_desc;
			xfer->port_num = port->port_num;
			xfer->cdc_num = port_id;
			xfer->token = USBH_PID_DATA;
			xfer->buffer = port->cache;
			xfer->trans_id = udev->trans;
			xfer->nbytes = wr_size;
			xfer->callback = quec_uhc_tx_callback;

			ret = udev->api->enqueue(udev->device, xfer);
			if(ret < 0)
			{
				quec_print("tx fail port %d size %d", port_id, size);
				port->is_busy = false;
				irq_unlock(irq_hd);
				return -1;
			}
		}
	}
	
	irq_unlock(irq_hd);
	return tx_size;
}

int quec_uhc_read_aviable(const struct device *dev, quec_cdc_port_e port_id)
{
	quec_uhc_mgr_t *udev = (quec_uhc_mgr_t *)dev->data;
	int rec_size = 0;
	
	if(udev == NULL || (port_id != QUEC_AT_PORT && port_id != QUEC_MODEM_PORT))
	{
		quec_print("param err %p %d", udev, port_id);
		return -1;
	}

	uint32_t irq_hd = irq_lock();
	
	quec_uhc_dev_t *uhc_port = &udev->dev[port_id];
	quec_uhc_pmg_t *port = &uhc_port->rx_port;
	if(uhc_port->status != QUEC_PORT_STATUS_OPEN)
	{
		quec_print("port %d status err %d %p", port_id, uhc_port->status, uhc_port);
		irq_unlock(irq_hd);
		return -1;
	}

	rec_size = ring_buffer_num_items(&port->fifo);
	
	irq_unlock(irq_hd);
	return rec_size;
}

int quec_uhc_write_aviable(const struct device *dev, quec_cdc_port_e port_id)
{
	quec_uhc_mgr_t *udev = (quec_uhc_mgr_t *)dev->data;
	int tx_size = 0;
	
	if(udev == NULL || (port_id != QUEC_AT_PORT && port_id != QUEC_MODEM_PORT))
	{
		return -1;
	}

	uint32_t irq_hd = irq_lock();

	quec_uhc_dev_t *uhc_port = &udev->dev[port_id];
	quec_uhc_pmg_t *port = &uhc_port->tx_port;
	if(uhc_port->status != QUEC_PORT_STATUS_OPEN)
	{
		quec_print("port %d status err %d %p", port_id, uhc_port->status, uhc_port);
		irq_unlock(irq_hd);
		return -1;
	}

	tx_size = ring_buffer_free_size(&port->fifo);

	irq_unlock(irq_hd);
	return tx_size;
}

int quec_uhc_ioctl(const struct device *dev, quec_ioctl_cmd_e cmd, void *param)
{
	quec_uhc_mgr_t *udev = (quec_uhc_mgr_t *)dev->data;

	switch(cmd)
	{
		case QUEC_GET_DEVICE_STATUS:
		{
			uint32_t irq_hd = irq_lock();	
			int status = udev->status;
			irq_unlock(irq_hd);

			return status;
		}

		case QUEC_SET_USER_CALLBACK:
		{
			uint32_t irq_hd = irq_lock();	
			udev->user_callback = (quec_uhc_callback)param;
			irq_unlock(irq_hd);

			return 0;
		}

		default:
			quec_print("cmd %d not support", cmd);
		return -1;
	}

}

static const quec_uhc_api_t quec_uhc_api = {
	.open = quec_uhc_open,
	.read = quec_uhc_read,
	.write = quec_uhc_write,
	.close = quec_uhc_close,
	.ioctl = quec_uhc_ioctl,
	.read_aviable = quec_uhc_read_aviable,
	.write_aviable = quec_uhc_write_aviable,
};


DEVICE_DEFINE(QUEC_UHC_DRIVER_ID, QUEC_UHC_DRIVER_NAME, quec_uhc_start,
					NULL, &udv_manager, NULL,
					POST_KERNEL, 99,
					&quec_uhc_api);
