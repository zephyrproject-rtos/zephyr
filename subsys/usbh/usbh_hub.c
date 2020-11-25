/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file is based on usbh_hub.c from uC/Modbus Stack.
 *
 *                                uC/Modbus
 *                         The Embedded USB Host Stack
 *
 *      Copyright 2003-2020 Silicon Laboratories Inc. www.silabs.com
 *
 *                   SPDX-License-Identifier: APACHE-2.0
 *
 * This software is subject to an open source license and is distributed by
 *  Silicon Laboratories Inc. pursuant to the terms of the Apache License,
 *      Version 2.0 available at www.apache.org/licenses/LICENSE-2.0.
 */

#include <usbh_hub.h>
#include <usbh_core.h>
#include <usbh_class.h>
#include <sys/byteorder.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(usbh_hub, CONFIG_USBH_LOG_LEVEL);

/*
 * ROOT HUB DEVICE DESCRIPTOR
 */
static const uint8_t usbh_hub_rh_dev_desc[18] = {
	USBH_LEN_DESC_DEV,              /* b_length */
	USBH_DESC_TYPE_DEV,             /* b_desc_type: Device */
	0x10u,
	0x01u,                          /* bcd_usb: v1.1 */
	USBH_CLASS_CODE_HUB,            /* b_device_class: HUB_CLASSCODE */
	USBH_SUBCLASS_CODE_USE_IF_DESC, /* b_device_sub_class */
	USBH_PROTOCOL_CODE_USE_IF_DESC, /* b_device_protocol */
	0x40u,                          /* b_max_packet_size_zero: 64 Bytes */
	0x00u,
	0x00u,                          /* id_vendor */
	0x00u,
	0x00u,                          /* id_product */
	0x00u,
	0x00u,                          /* bcd_device */
	0x00u,                          /* i_manufacturer */
	0x00u,                          /* i_product */
	0x00u,                          /* i_serial_number */
	0x01u                           /* bNumConfigurations */
};

/*
 * ROOT HUB CONFIGURATION DESCRIPTOR
 */
static const uint8_t usbh_hub_rh_fs_cfg_desc[] = {
	/*  CONFIGURATION DESCRIPTOR  */
	USBH_LEN_DESC_CFG,      /* b_length */
	USBH_DESC_TYPE_CFG,     /* b_desc_type CONFIGURATION */
	0x19u, 0x00u,           /* le16 w_total_length */
	0x01u,                  /* bNumInterfaces */
	0x01u,                  /* b_cfg_value */
	0x00u,                  /* i_cfg */
	0xC0u,                  /* bm_attributes->Self-powered|Remote wakeup */
	0x00u,                  /* b_max_pwr */

	/*  INTERFACE DESCRIPTOR  */
	USBH_LEN_DESC_IF,       /* b_length */
	USBH_DESC_TYPE_IF,      /* b_desc_type: Interface */
	0x00u,                  /* b_if_nbr */
	0x00u,                  /* b_alt_setting */
	0x01u,                  /* bNumEndpoints */
	USBH_CLASS_CODE_HUB,    /* b_if_class HUB_CLASSCODE */
	0x00u,                  /* b_if_sub_class */
	0x00u,                  /* b_if_protocol */
	0x00u,                  /* i_interface */

	/*  ENDPOINT DESCRIPTOR  */
	USBH_LEN_DESC_EP,       /* b_length */
	USBH_DESC_TYPE_EP,      /* b_desc_type: Endpoint */
	0x81u,                  /* b_endpoint_address: IN Endpoint 1 */
	0x03u,                  /* bm_attributes Interrupt */
	0x08u, 0x00u,           /* w_max_packet_size */
	0x1u                    /* b_interval */
};

/*
 * ROOT HUB STRING DESCRIPTOR
 */
static const uint8_t usbh_hub_rh_lang_id[] = {
	0x04u, USBH_DESC_TYPE_STR, 0x09u,
	0x04u, /* Identifer for English (United states). */
};

static uint8_t usbh_hub_desc_buf[USBH_HUB_MAX_DESC_LEN];
static struct usbh_hub_dev usbh_hub_arr[USBH_CFG_MAX_HUBS];
static int8_t hub_count = USBH_CFG_MAX_HUBS;
static volatile struct usbh_hub_dev *usbh_hub_head_ptr;
static volatile struct usbh_hub_dev *usbh_hub_tail_ptr;
static struct k_sem usbh_hub_event_sem;

static void usbh_hub_class_init(int *p_err);

static void *usbh_hub_if_probe(struct usbh_dev *p_dev, struct usbh_if *p_if,
			       int *p_err);

static void usbh_hub_suspend(void *p_class_dev);

static void usbh_hub_resume(void *p_class_dev);

static void usbh_hub_disconn(void *p_class_dev);

static int usbh_hub_init(struct usbh_hub_dev *p_hub_dev);

static void usbh_hub_uninit(struct usbh_hub_dev *p_hub_dev);

static int usbh_hub_ep_open(struct usbh_hub_dev *p_hub_dev);

static void usbh_hub_ep_close(struct usbh_hub_dev *p_hub_dev);

static int usbh_hub_event_req(struct usbh_hub_dev *p_hub_dev);

static void usbh_hub_isr_cb(struct usbh_ep *p_ep, void *p_buf, uint32_t buf_len,
			    uint32_t xfer_len, void *p_arg, int err);

static void usbh_hub_event_proc(void);

static int usbh_hub_desc_get(struct usbh_hub_dev *p_hub_dev);

static int usbh_hub_ports_init(struct usbh_hub_dev *p_hub_dev);

static int usbh_hub_port_status_get(struct usbh_hub_dev *p_hub_dev,
				    uint16_t port_nbr,
				    struct usbh_hub_port_status *p_port_status);

static int usbh_hub_port_reset_set(struct usbh_hub_dev *p_hub_dev,
				   uint16_t port_nbr);

static int usbh_hub_port_rst_chng_clr(struct usbh_hub_dev *p_hub_dev,
				      uint16_t port_nbr);

static int usbh_hub_port_en_chng_clr(struct usbh_hub_dev *p_hub_dev,
				     uint16_t port_nbr);

static int usbh_hub_port_conn_chng_clr(struct usbh_hub_dev *p_hub_dev,
				       uint16_t port_nbr);

static int usbh_hub_port_pwr_set(struct usbh_hub_dev *p_hub_dev,
				 uint16_t port_nbr);

static int usbh_hub_port_susp_clr(struct usbh_hub_dev *p_hub_dev,
				  uint16_t port_nbr);

static int usbh_hub_port_en_clr(struct usbh_hub_dev *p_hub_dev,
				uint16_t port_nbr);

static int usbh_hub_port_en_set(struct usbh_hub_dev *p_hub_dev,
				uint16_t port_nbr);

static void usbh_hub_clr(struct usbh_hub_dev *p_hub_dev);

static int usbh_hub_ref_add(struct usbh_hub_dev *p_hub_dev);

static int usbh_hub_ref_rel(struct usbh_hub_dev *p_hub_dev);

/*
 * HUB CLASS DRIVER
 */
const struct usbh_class_drv usbh_hub_drv = {
	(uint8_t *)"HUB",  usbh_hub_class_init, NULL,
	usbh_hub_if_probe, usbh_hub_suspend,    usbh_hub_resume,
	usbh_hub_disconn
};

/*
 * Task that process HUB Events.
 */
void usbh_hub_event_task(void *p_arg, void *p_arg2, void *p_arg3)
{
	while (1) {
		k_sem_take(&usbh_hub_event_sem, K_FOREVER);
		usbh_hub_event_proc();
	}
}

/*
 * Disable given port on hub.
 */
int usbh_hub_port_dis(struct usbh_hub_dev *p_hub_dev, uint16_t port_nbr)
{
	int err;

	if (p_hub_dev == NULL) {
		return -EINVAL;
	}

	err = usbh_hub_port_en_clr(p_hub_dev, port_nbr);

	return err;
}

/*
 * Enable given port on hub.
 */
int usbh_hub_port_en(struct usbh_hub_dev *p_hub_dev, uint16_t port_nbr)
{
	int err;

	if (p_hub_dev == NULL) {
		return -EINVAL;
	}

	err = usbh_hub_port_en_set(p_hub_dev, port_nbr);

	return err;
}

/*
 * Initializes all USBH_HUB_DEV structures, device lists and HUB pool.
 */
static void usbh_hub_class_init(int *p_err)
{
	uint8_t hub_ix;

	for (hub_ix = 0; hub_ix < USBH_CFG_MAX_HUBS; hub_ix++) {
		/* Clr all HUB dev structs. */
		usbh_hub_clr(&usbh_hub_arr[hub_ix]);
	}
	hub_count = (USBH_CFG_MAX_HUBS - 1);

	*p_err = k_sem_init(&usbh_hub_event_sem, 0, USBH_OS_SEM_REQUIRED);

	usbh_hub_head_ptr = NULL;
	usbh_hub_tail_ptr = NULL;

	memset(usbh_hub_desc_buf, 0, USBH_HUB_MAX_DESC_LEN);
}

/*
 * Determine whether connected device implements hub class
 * by examining it's interface descriptor.
 */
static void *usbh_hub_if_probe(struct usbh_dev *p_dev, struct usbh_if *p_if,
			       int *p_err)
{
	LOG_INF("hub if");
	struct usbh_hub_dev *p_hub_dev;
	struct usbh_if_desc if_desc;

	p_hub_dev = NULL;
	/* Get IF desc. */
	*p_err = usbh_if_desc_get(p_if, 0, &if_desc);
	if (*p_err != 0) {
		return NULL;
	}

	if (if_desc.b_if_class == USBH_CLASS_CODE_HUB) {
		/* If IF is HUB, alloc HUB dev. */
		if (hub_count < 0) {
			*p_err = EAGAIN;
			return NULL;
		}

		p_hub_dev = &usbh_hub_arr[hub_count--];

		usbh_hub_clr(p_hub_dev);
		usbh_hub_ref_add(p_hub_dev);

		p_hub_dev->state = (uint8_t)USBH_CLASS_DEV_STATE_CONN;
		p_hub_dev->dev_ptr = p_dev;
		p_hub_dev->if_ptr = p_if;
		p_hub_dev->err_cnt = 0;

		if ((p_dev->is_root_hub == true) &&
		    (p_dev->hc_ptr->is_vir_rh == true)) {
			p_dev->hc_ptr->rh_class_dev_ptr = p_hub_dev;
		}
		/* Init HUB. */
		*p_err = usbh_hub_init(p_hub_dev);
		if (*p_err != 0) {
			usbh_hub_ref_rel(p_hub_dev);
		}
	} else {
		*p_err = EAGAIN;
	}

	return ((void *)p_hub_dev);
}

/*
 * Suspend given hub and all devices connected to it.
 */
static void usbh_hub_suspend(void *p_class_dev)
{
	uint16_t nbr_ports;
	uint16_t port_ix;
	struct usbh_dev *p_dev;
	struct usbh_hub_dev *p_hub_dev;

	p_hub_dev = (struct usbh_hub_dev *)p_class_dev;
	nbr_ports = MIN(p_hub_dev->desc.b_nbr_ports, USBH_CFG_MAX_HUB_PORTS);

	for (port_ix = 0; port_ix < nbr_ports; port_ix++) {
		p_dev = (struct usbh_dev *)p_hub_dev->dev_ptr_list[port_ix];

		if (p_dev != NULL) {
			usbh_class_suspend(p_dev);
		}
	}
}

/*
 * Resume given hub and all devices connected to it.
 */
static void usbh_hub_resume(void *p_class_dev)
{
	uint16_t nbr_ports;
	uint16_t port_ix;
	struct usbh_dev *p_dev;
	struct usbh_hub_dev *p_hub_dev;
	struct usbh_hub_port_status port_status;

	p_hub_dev = (struct usbh_hub_dev *)p_class_dev;
	nbr_ports = MIN(p_hub_dev->desc.b_nbr_ports, USBH_CFG_MAX_HUB_PORTS);

	for (port_ix = 0; port_ix < nbr_ports; port_ix++) {
		/* En resume signaling on port. */
		usbh_hub_port_susp_clr(p_hub_dev, port_ix + 1);
	}

	k_sleep(K_MSEC(20u + 12u));

	for (port_ix = 0; port_ix < nbr_ports; port_ix++) {
		p_dev = p_hub_dev->dev_ptr_list[port_ix];

		if (p_dev != NULL) {
			usbh_class_resume(p_dev);
		} else {
			/* Get port status info. */
			usbh_hub_port_status_get(p_hub_dev, port_ix + 1,
						 &port_status);

			if ((port_status.w_port_status &
			     USBH_HUB_STATUS_PORT_CONN) != 0) {
				usbh_hub_port_reset_set(p_hub_dev, port_ix + 1);
			}
		}
	}
}

/*
 * Disconnect given hub.
 */
static void usbh_hub_disconn(void *p_class_dev)
{
	struct usbh_hub_dev *p_hub_dev;

	p_hub_dev = (struct usbh_hub_dev *)p_class_dev;
	p_hub_dev->state = USBH_CLASS_DEV_STATE_DISCONN;

	usbh_hub_uninit(p_hub_dev);
	usbh_hub_ref_rel(p_hub_dev);
}

/*
 * Opens the endpoints, reads hub descriptor, initializes ports
 * and submits request to start receiving hub events.
 */
static int usbh_hub_init(struct usbh_hub_dev *p_hub_dev)
{
	int err;

	/* Open intr EP. */
	err = usbh_hub_ep_open(p_hub_dev);
	if (err != 0) {
		return err;
	}
	/* Get hub desc. */
	err = usbh_hub_desc_get(p_hub_dev);
	if (err != 0) {
		return err;
	}
	/* Init hub ports. */
	err = usbh_hub_ports_init(p_hub_dev);
	if (err != 0) {
		return err;
	}
	/* Start receiving hub evts. */
	err = usbh_hub_event_req(p_hub_dev);

	return err;
}

/*
 * Uninitialize given hub.
 */
static void usbh_hub_uninit(struct usbh_hub_dev *p_hub_dev)
{
	uint16_t nbr_ports;
	uint16_t port_ix;
	struct usbh_dev *p_dev;

	usbh_hub_ep_close(p_hub_dev);
	nbr_ports = MIN(p_hub_dev->desc.b_nbr_ports, USBH_CFG_MAX_HUB_PORTS);

	for (port_ix = 0; port_ix < nbr_ports; port_ix++) {
		p_dev = (struct usbh_dev *)p_hub_dev->dev_ptr_list[port_ix];

		if (p_dev != NULL) {
			usbh_dev_disconn(p_dev);
			p_hub_dev->dev_ptr->hc_ptr->host_ptr->dev_cnt++;

			p_hub_dev->dev_ptr_list[port_ix] = NULL;
		}
	}
}

/*
 * Open interrupt endpoint required to receive hub events.
 */
static int usbh_hub_ep_open(struct usbh_hub_dev *p_hub_dev)
{
	int err;
	struct usbh_dev *p_dev;
	struct usbh_if *p_if;

	p_dev = p_hub_dev->dev_ptr;
	p_if = p_hub_dev->if_ptr;
	/* Find and open hub intr EP. */
	err = usbh_intr_in_open(p_dev, p_if, &p_hub_dev->intr_ep);

	return err;
}

/*
 * Close interrupt endpoint.
 */
static void usbh_hub_ep_close(struct usbh_hub_dev *p_hub_dev)
{
	/* Close hub intr EP. */
	usbh_ep_close(&p_hub_dev->intr_ep);
}

/*
 * Issue an asynchronous interrupt request to receive hub events.
 */
static int usbh_hub_event_req(struct usbh_hub_dev *p_hub_dev)
{
	uint32_t len;
	bool valid;
	const struct usbh_hc_rh_api *p_rh_api;
	int err;
	struct usbh_dev *p_dev;

	p_dev = p_hub_dev->dev_ptr;
	/* Chk if RH fncts are supported before calling HCD. */
	if ((p_dev->is_root_hub == true) &&
	    (p_dev->hc_ptr->is_vir_rh == true)) {
		p_rh_api = p_dev->hc_ptr->hc_drv.rh_api_ptr;
		valid = p_rh_api->int_en(&p_dev->hc_ptr->hc_drv);

		if (valid != 1) {
			return -EIO;
		} else {
			return 0;
		}
	}

	len = (p_hub_dev->desc.b_nbr_ports / 8) + 1;
	/* Start receiving hub events. */
	err = usbh_intr_rx_async(&p_hub_dev->intr_ep,
				 (void *)p_hub_dev->hub_intr_buf, len,
				 usbh_hub_isr_cb, (void *)p_hub_dev);
	return err;
}

/*
 * Handles hub interrupt.
 */
static void usbh_hub_isr_cb(struct usbh_ep *p_ep, void *p_buf, uint32_t buf_len,
			    uint32_t xfer_len, void *p_arg, int err)
{
	struct usbh_hub_dev *p_hub_dev;
	int key;

	(void)buf_len;
	(void)p_buf;
	(void)p_ep;
	(void)xfer_len;

	p_hub_dev = (struct usbh_hub_dev *)p_arg;

	if (err != 0) {
		if (p_hub_dev->state == USBH_CLASS_DEV_STATE_CONN) {
			if (p_hub_dev->err_cnt < 3u) {
				LOG_ERR("%s fails. err=%d errcnt=%d\r\n",
					__func__, err, p_hub_dev->err_cnt);

				p_hub_dev->err_cnt++;
				/* Retry urb. */
				usbh_hub_event_req(p_hub_dev);
			}
		}
		return;
	}

	p_hub_dev->err_cnt = 0;

	usbh_hub_ref_add(p_hub_dev);

	key = irq_lock();
	if (usbh_hub_head_ptr == NULL) {
		usbh_hub_head_ptr = usbh_hub_tail_ptr = p_hub_dev;
	} else {
		usbh_hub_tail_ptr->nxt_ptr = p_hub_dev;
		usbh_hub_tail_ptr = p_hub_dev;
	}
	irq_unlock(key);

	k_sem_give(&usbh_hub_event_sem);
}

int conn_err_routine(struct usbh_hub_dev *p_hub_dev, struct usbh_dev *p_dev,
		     uint16_t *port_nbr, int *err)
{
	usbh_hub_port_dis(p_hub_dev, *port_nbr);
	usbh_dev_disconn(p_dev);

	p_hub_dev->dev_ptr->hc_ptr->host_ptr->dev_cnt++;

	if (p_hub_dev->conn_cnt < USBH_CFG_MAX_NUM_DEV_RECONN) {
		/*This condition may happen due to EP_STALL return */
		/* Apply port reset. */
		*err = usbh_hub_port_reset_set(p_hub_dev, *port_nbr);
		if (*err != 0) {
			return -1;
		}

		k_sleep(K_MSEC(USBH_HUB_DLY_DEV_RESET));
		p_hub_dev->conn_cnt++;
		/* Handle port reset status change. */
		return 0;
	}

	p_hub_dev->dev_ptr_list[*port_nbr - 1] = NULL;

	return 1;
}

/*
 * Determine status of each of hub ports. Newly connected device will be
 * reset and configured. Appropriate notifications & cleanup will be
 * performed if a device has been disconnected.
 */
static void usbh_hub_event_proc(void)
{
	uint16_t nbr_ports;
	uint16_t port_nbr;
	uint8_t dev_spd;
	struct usbh_hub_dev *p_hub_dev;
	struct usbh_hub_port_status port_status;
	struct usbh_dev *p_dev;
	int err;
	int key;

	key = irq_lock();
	p_hub_dev = (struct usbh_hub_dev *)usbh_hub_head_ptr;

	if (usbh_hub_head_ptr == usbh_hub_tail_ptr) {
		usbh_hub_head_ptr = NULL;
		usbh_hub_tail_ptr = NULL;
	} else {
		usbh_hub_head_ptr = usbh_hub_head_ptr->nxt_ptr;
	}
	irq_unlock(key);

	if (p_hub_dev == NULL) {
		return;
	}

	if (p_hub_dev->state == USBH_CLASS_DEV_STATE_DISCONN) {
		LOG_DBG("device state disconnected");
		err = usbh_hub_ref_rel(p_hub_dev);
		if (err != 0) {
			LOG_ERR("could not release reference %d", err);
		}
		return;
	}

	port_nbr = 1;
	nbr_ports = MIN(p_hub_dev->desc.b_nbr_ports, USBH_CFG_MAX_HUB_PORTS);

	while (port_nbr <= nbr_ports) {
		/* Get port status info.. */
		err = usbh_hub_port_status_get(p_hub_dev, port_nbr,
					       &port_status);
		if (err != 0) {
			break;
		}
		/* CONNECTION STATUS CHANGE */
		if (DEF_BIT_IS_SET(port_status.w_port_change,
				   USBH_HUB_STATUS_C_PORT_CONN) == true) {
			LOG_DBG("connection status change");
			/* Clr port conn chng. */
			err = usbh_hub_port_conn_chng_clr(p_hub_dev, port_nbr);
			if (err != 0) {
				break;
			}
			/*  DEV HAS BEEN CONNECTED  */
			if (DEF_BIT_IS_SET(port_status.w_port_status,
					   USBH_HUB_STATUS_PORT_CONN) == true) {
				LOG_DBG("Port %d : Device Connected.\r\n",
					port_nbr);
				/* Reset re-connection counter */
				p_hub_dev->conn_cnt = 0;
				p_dev = p_hub_dev->dev_ptr_list[port_nbr - 1];
				if (p_dev != NULL) {
					usbh_dev_disconn(p_dev);
					p_hub_dev->dev_ptr->hc_ptr->host_ptr
					->dev_cnt++;
					p_hub_dev->dev_ptr_list[port_nbr - 1] =
						NULL;
				}

				k_sleep(K_MSEC(100u));
				/* Apply port reset. */
				err = usbh_hub_port_reset_set(p_hub_dev,
							      port_nbr);
				if (err != 0) {
					break;
				}
				/* Handle port reset status change. */
				k_sleep(K_MSEC(USBH_HUB_DLY_DEV_RESET));
				continue;
			} else {
				/* DEV HAS BEEN REMOVED */
				LOG_DBG("device has been removed");
				/* Wait for any pending I/O xfer to rtn err. */
				k_sleep(K_MSEC(10u));

				p_dev = p_hub_dev->dev_ptr_list[port_nbr - 1];

				if (p_dev != NULL) {
					usbh_dev_disconn(p_dev);
					p_hub_dev->dev_ptr->hc_ptr->host_ptr
					->dev_cnt++;

					p_hub_dev->dev_ptr_list[port_nbr - 1] =
						NULL;
				}
			}
		}
		/*  PORT RESET STATUS CHANGE  */
		if (DEF_BIT_IS_SET(port_status.w_port_change,
				   USBH_HUB_STATUS_C_PORT_RESET) == true) {
			err = usbh_hub_port_rst_chng_clr(p_hub_dev, port_nbr);
			if (err != 0) {
				break;
			}
			/* Dev has been connected.*/
			if (DEF_BIT_IS_SET(port_status.w_port_status,
					   USBH_HUB_STATUS_PORT_CONN) == true) {
				/* Get port status info.*/
				err = usbh_hub_port_status_get(
					p_hub_dev, port_nbr, &port_status);
				if (err != 0) {
					break;
				}

				/* Determine dev spd. */
				if (DEF_BIT_IS_SET(
					    port_status.w_port_status,
					    USBH_HUB_STATUS_PORT_LOW_SPD) ==
				    true) {
					dev_spd = USBH_LOW_SPEED;
				} else if (DEF_BIT_IS_SET(
						   port_status.w_port_status,
						   USBH_HUB_STATUS_PORT_HIGH_SPD
						   ) == true) {
					dev_spd = USBH_HIGH_SPEED;
				} else {
					dev_spd = USBH_FULL_SPEED;
				}

				LOG_DBG("Port %d : Port Reset complete, device speed is %s",
					port_nbr,
					(dev_spd == USBH_LOW_SPEED) ?
					"LOW Speed(1.5 Mb/Sec)" :
					(dev_spd == USBH_FULL_SPEED) ?
					"FULL Speed(12 Mb/Sec)" :
					"HIGH Speed(480 Mb/Sec)");

				p_dev = p_hub_dev->dev_ptr_list[port_nbr - 1];

				if (p_dev != NULL) {
					continue;
				}

				if (p_hub_dev->dev_ptr->hc_ptr->host_ptr->state
				    == USBH_HOST_STATE_SUSPENDED) {
					continue;
				}
				if (p_hub_dev->dev_ptr->hc_ptr->host_ptr
				    ->dev_cnt < 0) {
					usbh_hub_port_dis(p_hub_dev, port_nbr);
					usbh_hub_ref_rel(p_hub_dev);
					/* Retry urb. */
					usbh_hub_event_req(p_hub_dev);

					return;
				}
				p_dev = &p_hub_dev->dev_ptr->hc_ptr->host_ptr
					->dev_list[p_hub_dev->dev_ptr
						   ->hc_ptr
						   ->host_ptr
						   ->dev_cnt--];

				p_dev->dev_spd = dev_spd;
				p_dev->hub_dev_ptr = p_hub_dev->dev_ptr;
				p_dev->port_nbr = port_nbr;
				p_dev->hc_ptr = p_hub_dev->dev_ptr->hc_ptr;

				if (dev_spd == USBH_HIGH_SPEED) {
					p_dev->hub_hs_ptr = p_hub_dev;
				} else {
					if (p_hub_dev->intr_ep.dev_spd ==
					    USBH_HIGH_SPEED) {
						p_dev->hub_hs_ptr = p_hub_dev;
					} else {
						p_dev->hub_hs_ptr =
							p_hub_dev->dev_ptr
							->hub_hs_ptr;
					}
				}

				k_sleep(K_MSEC(50u));
				/* Conn dev. */
				err = usbh_dev_conn(p_dev);
				if (err != 0) {
					int ret = conn_err_routine(p_hub_dev,
								   p_dev,
								   &port_nbr,
								   &err);

					if (ret < 0) {
						break;
					} else if (ret == 0) {
						continue;
					}
				} else {
					p_hub_dev->dev_ptr_list[port_nbr - 1] =
						p_dev;
				}
			}
		}
		/* PORT ENABLE STATUS CHANGE */
		if (DEF_BIT_IS_SET(port_status.w_port_change,
				   USBH_HUB_STATUS_C_PORT_EN) == true) {
			err = usbh_hub_port_en_chng_clr(p_hub_dev, port_nbr);
			if (err != 0) {
				break;
			}
		}
		port_nbr++;
	}
	/* Retry urb. */
	usbh_hub_event_req(p_hub_dev);

	usbh_hub_ref_rel(p_hub_dev);
}

/*
 * Retrieve hub descriptor.
 */
static int usbh_hub_desc_get(struct usbh_hub_dev *p_hub_dev)
{
	int err;
	uint32_t len;
	uint32_t i;
	struct usbh_desc_hdr hdr;

	for (i = 0; i < USBH_CFG_STD_REQ_RETRY; i++) {
		/* Attempt to get desc hdr 3 times. */
		len = usbh_ctrl_rx(
			p_hub_dev->dev_ptr, USBH_REQ_GET_DESC,
			(USBH_REQ_DIR_DEV_TO_HOST | USBH_REQ_TYPE_CLASS),
			(USBH_HUB_DESC_TYPE_HUB << 8), 0, (void *)&hdr,
			USBH_LEN_DESC_HDR, USBH_HUB_TIMEOUT, &err);
		if ((err == EBUSY) || (len == 0)) {
			usbh_ep_reset(p_hub_dev->dev_ptr, NULL);
		} else {
			break;
		}
	}

	if (len != USBH_LEN_DESC_HDR) {
		return -EINVAL;
	}

	if ((hdr.b_length == 0) || (hdr.b_length > USBH_HUB_MAX_DESC_LEN) ||
	    (hdr.b_desc_type != USBH_HUB_DESC_TYPE_HUB)) {
		return -EINVAL;
	}

	for (i = 0; i < USBH_CFG_STD_REQ_RETRY; i++) {
		/* Attempt to get full desc 3 times. */
		len = usbh_ctrl_rx(p_hub_dev->dev_ptr, USBH_REQ_GET_DESC,
				   (USBH_REQ_DIR_DEV_TO_HOST |
				    USBH_REQ_TYPE_CLASS),
				   (USBH_HUB_DESC_TYPE_HUB << 8), 0,
				   (void *)usbh_hub_desc_buf, hdr.b_length,
				   USBH_HUB_TIMEOUT, &err);
		if ((err == EBUSY) || (len < hdr.b_length)) {
			usbh_ep_reset(p_hub_dev->dev_ptr, NULL);
		} else {
			break;
		}
	}

	usbh_hub_parse_hub_desc(&p_hub_dev->desc, usbh_hub_desc_buf);

	if (p_hub_dev->desc.b_nbr_ports > USBH_CFG_MAX_HUB_PORTS) {
		/* Warns limit on hub port nbr to max cfg'd. */
		LOG_WRN("Only ports [1..%d] are active.\r\n",
			USBH_CFG_MAX_HUB_PORTS);
	}

	return err;
}

/*
 * Enable power on each hub port & initialize device on each port.
 */
static int usbh_hub_ports_init(struct usbh_hub_dev *p_hub_dev)
{
	int err;
	uint32_t i;
	uint16_t nbr_ports;

	nbr_ports = MIN(p_hub_dev->desc.b_nbr_ports, USBH_CFG_MAX_HUB_PORTS);

	for (i = 0; i < nbr_ports; i++) {
		/* Set port pwr. */
		err = usbh_hub_port_pwr_set(p_hub_dev, i + 1);

		if (err != 0) {
			LOG_ERR("PortPwrSet error");
			return err;
		}
		k_sleep(K_MSEC(p_hub_dev->desc.b_pwr_on_to_pwr_good * 2));
	}
	return 0;
}

/*
 * Get port status on given hub.
 */
static int usbh_hub_port_status_get(struct usbh_hub_dev *p_hub_dev,
				    uint16_t port_nbr,
				    struct usbh_hub_port_status *p_port_status)
{
	uint8_t *p_buf;
	struct usbh_hub_port_status port_status;
	int err;

	p_buf = (uint8_t *)&port_status;

	usbh_ctrl_rx(p_hub_dev->dev_ptr, USBH_REQ_GET_STATUS,
		     (USBH_REQ_DIR_DEV_TO_HOST | USBH_REQ_TYPE_CLASS |
		      USBH_REQ_RECIPIENT_OTHER),
		     0, port_nbr, (void *)p_buf, USBH_HUB_LEN_HUB_PORT_STATUS,
		     USBH_HUB_TIMEOUT, &err);
	if (err != 0) {
		usbh_ep_reset(p_hub_dev->dev_ptr, NULL);
	} else {
		p_port_status->w_port_status =
			sys_get_le16((uint8_t *)&port_status.w_port_status);
		p_port_status->w_port_change =
			sys_get_le16((uint8_t *)&port_status.w_port_change);
	}

	return err;
}

/*
 * Set port reset on given hub.
 */
static int usbh_hub_port_reset_set(struct usbh_hub_dev *p_hub_dev,
				   uint16_t port_nbr)
{
	int err;

	usbh_ctrl_tx(p_hub_dev->dev_ptr, USBH_REQ_SET_FEATURE,
		     (USBH_REQ_DIR_HOST_TO_DEV | USBH_REQ_TYPE_CLASS |
		      USBH_REQ_RECIPIENT_OTHER),
		     USBH_HUB_FEATURE_SEL_PORT_RESET, port_nbr, NULL, 0,
		     USBH_HUB_TIMEOUT, &err);
	if (err != 0) {
		usbh_ep_reset(p_hub_dev->dev_ptr, NULL);
	}

	return err;
}

/*
 * Clear port reset change on given hub.
 */
static int usbh_hub_port_rst_chng_clr(struct usbh_hub_dev *p_hub_dev,
				      uint16_t port_nbr)
{
	int err;

	usbh_ctrl_tx(p_hub_dev->dev_ptr, USBH_REQ_CLR_FEATURE,
		     (USBH_REQ_DIR_HOST_TO_DEV | USBH_REQ_TYPE_CLASS |
		      USBH_REQ_RECIPIENT_OTHER),
		     USBH_HUB_FEATURE_SEL_C_PORT_RESET, port_nbr, NULL, 0,
		     USBH_HUB_TIMEOUT, &err);
	if (err != 0) {
		usbh_ep_reset(p_hub_dev->dev_ptr, NULL);
	}

	return err;
}

/*
 * Clear port enable change on given hub.
 */
static int usbh_hub_port_en_chng_clr(struct usbh_hub_dev *p_hub_dev,
				     uint16_t port_nbr)
{
	int err;

	usbh_ctrl_tx(p_hub_dev->dev_ptr, USBH_REQ_CLR_FEATURE,
		     (USBH_REQ_DIR_HOST_TO_DEV | USBH_REQ_TYPE_CLASS |
		      USBH_REQ_RECIPIENT_OTHER),
		     USBH_HUB_FEATURE_SEL_C_PORT_EN, port_nbr, NULL, 0,
		     USBH_HUB_TIMEOUT, &err);
	if (err != 0) {
		usbh_ep_reset(p_hub_dev->dev_ptr, NULL);
	}

	return err;
}

/*
 * Clear port connection change on given hub.
 */
static int usbh_hub_port_conn_chng_clr(struct usbh_hub_dev *p_hub_dev,
				       uint16_t port_nbr)
{
	int err;

	usbh_ctrl_tx(p_hub_dev->dev_ptr, USBH_REQ_CLR_FEATURE,
		     (USBH_REQ_DIR_HOST_TO_DEV | USBH_REQ_TYPE_CLASS |
		      USBH_REQ_RECIPIENT_OTHER),
		     USBH_HUB_FEATURE_SEL_C_PORT_CONN, port_nbr, NULL, 0,
		     USBH_HUB_TIMEOUT, &err);
	if (err != 0) {
		usbh_ep_reset(p_hub_dev->dev_ptr, NULL);
	}

	return err;
}

/*
 * Set power on given hub and port.
 */
static int usbh_hub_port_pwr_set(struct usbh_hub_dev *p_hub_dev,
				 uint16_t port_nbr)
{
	int err;

	usbh_ctrl_tx(p_hub_dev->dev_ptr, USBH_REQ_SET_FEATURE,
		     (USBH_REQ_DIR_HOST_TO_DEV | USBH_REQ_TYPE_CLASS |
		      USBH_REQ_RECIPIENT_OTHER),
		     USBH_HUB_FEATURE_SEL_PORT_PWR, port_nbr, NULL, 0,
		     USBH_HUB_TIMEOUT, &err);
	if (err != 0) {
		usbh_ep_reset(p_hub_dev->dev_ptr, NULL);
	}

	return err;
}

/*
 * Clear port suspend on given hub.
 */
static int usbh_hub_port_susp_clr(struct usbh_hub_dev *p_hub_dev,
				  uint16_t port_nbr)
{
	int err;

	usbh_ctrl_tx(p_hub_dev->dev_ptr, USBH_REQ_CLR_FEATURE,
		     (USBH_REQ_DIR_HOST_TO_DEV | USBH_REQ_TYPE_CLASS |
		      USBH_REQ_RECIPIENT_OTHER),
		     USBH_HUB_FEATURE_SEL_C_PORT_SUSPEND, port_nbr, NULL, 0,
		     USBH_HUB_TIMEOUT, &err);
	if (err != 0) {
		usbh_ep_reset(p_hub_dev->dev_ptr, NULL);
	}

	return err;
}

/*
 * Clear port enable on given hub.
 */
static int usbh_hub_port_en_clr(struct usbh_hub_dev *p_hub_dev,
				uint16_t port_nbr)
{
	int err;

	usbh_ctrl_tx(p_hub_dev->dev_ptr, USBH_REQ_CLR_FEATURE,
		     (USBH_REQ_DIR_HOST_TO_DEV | USBH_REQ_TYPE_CLASS |
		      USBH_REQ_RECIPIENT_OTHER),
		     USBH_HUB_FEATURE_SEL_PORT_EN, port_nbr, NULL, 0,
		     USBH_HUB_TIMEOUT, &err);
	if (err != 0) {
		usbh_ep_reset(p_hub_dev->dev_ptr, NULL);
	}

	return err;
}

/*
 * Set port enable on given hub.
 */
static int usbh_hub_port_en_set(struct usbh_hub_dev *p_hub_dev,
				uint16_t port_nbr)
{
	int err;

	usbh_ctrl_tx(p_hub_dev->dev_ptr, USBH_REQ_SET_FEATURE,
		     (USBH_REQ_DIR_HOST_TO_DEV | USBH_REQ_TYPE_CLASS |
		      USBH_REQ_RECIPIENT_OTHER),
		     USBH_HUB_FEATURE_SEL_PORT_EN, port_nbr, NULL, 0,
		     USBH_HUB_TIMEOUT, &err);
	if (err != 0) {
		usbh_ep_reset(p_hub_dev->dev_ptr, NULL);
	}

	return err;
}

/*
 * Set port suspend on given hub.
 */
int usbh_hub_port_suspend_set(struct usbh_hub_dev *p_hub_dev, uint16_t port_nbr)
{
	int err;

	usbh_ctrl_tx(p_hub_dev->dev_ptr, USBH_REQ_SET_FEATURE,
		     (USBH_REQ_DIR_HOST_TO_DEV | USBH_REQ_TYPE_CLASS |
		      USBH_REQ_RECIPIENT_OTHER),
		     USBH_HUB_FEATURE_SEL_PORT_SUSPEND, port_nbr, NULL, 0,
		     USBH_HUB_TIMEOUT, &err);
	if (err != 0) {
		usbh_ep_reset(p_hub_dev->dev_ptr, NULL);
	}

	return err;
}

/*
 * Initializes USBH_HUB_DEV structure.
 */
static void usbh_hub_clr(struct usbh_hub_dev *p_hub_dev)
{
	uint8_t dev_ix;

	p_hub_dev->dev_ptr = NULL;
	p_hub_dev->if_ptr = NULL;
	/* Clr dev ptr lst. */
	for (dev_ix = 0; dev_ix < USBH_CFG_MAX_HUB_PORTS; dev_ix++) {
		p_hub_dev->dev_ptr_list[dev_ix] = NULL;
	}

	p_hub_dev->ref_cnt = 0;
	p_hub_dev->state = USBH_CLASS_DEV_STATE_NONE;
	p_hub_dev->nxt_ptr = 0;
}

/*
 * Increment access reference count to a hub device.
 */
static int usbh_hub_ref_add(struct usbh_hub_dev *p_hub_dev)
{
	int key;

	if (p_hub_dev == NULL) {
		return -EINVAL;
	}

	key = irq_lock();
	/* Increment access ref cnt to hub dev. */
	p_hub_dev->ref_cnt++;

	irq_unlock(key);
	return 0;
}

/*
 * Increment the access reference count to a hub device.
 */
static int usbh_hub_ref_rel(struct usbh_hub_dev *p_hub_dev)
{
	int key;

	if (p_hub_dev == NULL) {
		return -EINVAL;
	}

	key = irq_lock();
	if (p_hub_dev->ref_cnt > 0) {
		/* Decrement access ref cnt to hub dev. */
		p_hub_dev->ref_cnt--;

		if (p_hub_dev->ref_cnt == 0) {
			hub_count++;
		}
	}
	irq_unlock(key);

	return 0;
}

/*
 * Process hub request from core layer.
 */
uint32_t usbh_rh_ctrl_req(struct usbh_hc *p_hc, uint8_t b_req,
			  uint8_t bm_req_type, uint16_t w_val, uint16_t w_ix,
			  void *p_buf, uint32_t buf_len, int *p_err)
{
	uint32_t len;
	struct usbh_hc_drv *p_hc_drv;
	const struct usbh_hc_rh_api *p_hc_rh_api;
	bool valid;

	p_hc_drv = &p_hc->hc_drv;
	p_hc_rh_api = p_hc_drv->rh_api_ptr;
	*p_err = 0;
	len = 0;
	valid = 1;

	switch (b_req) {
	case USBH_REQ_GET_STATUS:
		/* Only port status is supported. */
		if ((bm_req_type & USBH_REQ_RECIPIENT_OTHER) ==
		    USBH_REQ_RECIPIENT_OTHER) {
			valid = p_hc_rh_api->status_get(
				p_hc_drv, w_ix,
				(struct usbh_hub_port_status *)p_buf);
		} else {
			len = buf_len;
			/* Return 0 for other reqs. */
			memset(p_buf, 0, len);
		}
		break;

	case USBH_REQ_CLR_FEATURE:
		switch (w_val) {
		case USBH_HUB_FEATURE_SEL_PORT_EN:
			valid = p_hc_rh_api->en_clr(p_hc_drv, w_ix);
			break;

		case USBH_HUB_FEATURE_SEL_PORT_PWR:
			valid = p_hc_rh_api->pwr_clr(p_hc_drv, w_ix);
			break;

		case USBH_HUB_FEATURE_SEL_C_PORT_CONN:
			valid = p_hc_rh_api->conn_chng_clr(p_hc_drv, w_ix);
			break;

		case USBH_HUB_FEATURE_SEL_C_PORT_RESET:
			valid = p_hc_rh_api->rst_chng_clr(p_hc_drv, w_ix);
			break;

		case USBH_HUB_FEATURE_SEL_C_PORT_EN:
			valid = p_hc_rh_api->en_chng_clr(p_hc_drv, w_ix);
			break;

		case USBH_HUB_FEATURE_SEL_PORT_INDICATOR:
		case USBH_HUB_FEATURE_SEL_PORT_SUSPEND:
		case USBH_HUB_FEATURE_SEL_C_PORT_SUSPEND:
			valid = p_hc_rh_api->suspend_clr(p_hc_drv, w_ix);
			break;

		case USBH_HUB_FEATURE_SEL_C_PORT_OVER_CUR:
			*p_err = EBUSY;
			break;

		default:
			break;
		}
		break;

	case USBH_REQ_SET_FEATURE:
		switch (w_val) {
		case USBH_HUB_FEATURE_SEL_PORT_EN:
			valid = p_hc_rh_api->en_set(p_hc_drv, w_ix);
			break;

		case USBH_HUB_FEATURE_SEL_PORT_RESET:
			valid = p_hc_rh_api->rst_set(p_hc_drv, w_ix);
			break;

		case USBH_HUB_FEATURE_SEL_PORT_PWR:
			valid = p_hc_rh_api->pwr_set(p_hc_drv, w_ix);
			break;
		/* Not supported reqs. */
		case USBH_HUB_FEATURE_SEL_PORT_SUSPEND:
		case USBH_HUB_FEATURE_SEL_PORT_TEST:
		case USBH_HUB_FEATURE_SEL_PORT_INDICATOR:
		case USBH_HUB_FEATURE_SEL_C_PORT_CONN:
		case USBH_HUB_FEATURE_SEL_C_PORT_RESET:
		case USBH_HUB_FEATURE_SEL_C_PORT_EN:
		case USBH_HUB_FEATURE_SEL_C_PORT_SUSPEND:
		case USBH_HUB_FEATURE_SEL_C_PORT_OVER_CUR:
			*p_err = EBUSY;
			break;

		default:
			break;
		}
		break;

	case USBH_REQ_SET_ADDR:
		break;

	case USBH_REQ_GET_DESC:
		switch (w_val >> 8) {
		/* desc type. */
		case USBH_DESC_TYPE_DEV:
			if (buf_len > sizeof(usbh_hub_rh_dev_desc)) {
				len = sizeof(usbh_hub_rh_dev_desc);
			} else {
				len = buf_len;
			}

			memcpy(p_buf, (void *)usbh_hub_rh_dev_desc, len);
			break;

		case USBH_DESC_TYPE_CFG:
			/* Return cfg desc. */
			if (buf_len > sizeof(usbh_hub_rh_fs_cfg_desc)) {
				len = sizeof(usbh_hub_rh_fs_cfg_desc);
			} else {
				len = buf_len;
			}
			memcpy(p_buf, (void *)usbh_hub_rh_fs_cfg_desc, len);
			break;

		case USBH_HUB_DESC_TYPE_HUB:
			/* Return hub desc. */
			len = buf_len;
			valid = p_hc_rh_api->desc_get(
				p_hc_drv, (struct usbh_hub_desc *)p_buf, len);
			break;

		case USBH_DESC_TYPE_STR:

			if ((w_val & 0x00FF) == 0) {
				if (buf_len > sizeof(usbh_hub_rh_lang_id)) {
					len = sizeof(usbh_hub_rh_lang_id);
				} else {
					len = buf_len;
				}
				memcpy(p_buf, (void *)usbh_hub_rh_lang_id, len);
			} else {
				*p_err = EBUSY;
				break;
			}
			break;

		default:
			break;
		}
		break;

	case USBH_REQ_SET_CFG:
		break;

	case USBH_REQ_GET_CFG:
	case USBH_REQ_GET_IF:
	case USBH_REQ_SET_IF:
	case USBH_REQ_SET_DESC:
	case USBH_REQ_SYNCH_FRAME:
		*p_err = EBUSY;
		break;

	default:
		break;
	}

	if ((valid != 1) && (*p_err == 0)) {
		*p_err = -EIO;
	}

	return len;
}

/*
 * Queue a root hub event.
 */
void usbh_rh_event(struct usbh_dev *p_dev)
{
	struct usbh_hub_dev *p_hub_dev;
	struct usbh_hc_drv *p_hc_drv;
	const struct usbh_hc_rh_api *p_rh_drv_api;
	int key;

	p_hub_dev = p_dev->hc_ptr->rh_class_dev_ptr;
	p_hc_drv = &p_dev->hc_ptr->hc_drv;
	p_rh_drv_api = p_hc_drv->rh_api_ptr;
	if (p_hub_dev == NULL) {
		p_rh_drv_api->int_dis(p_hc_drv);
		return;
	}

	p_rh_drv_api->int_dis(p_hc_drv);

	usbh_hub_ref_add(p_hub_dev);

	key = irq_lock();
	if (usbh_hub_head_ptr == NULL) {
		usbh_hub_head_ptr = p_hub_dev;
		usbh_hub_tail_ptr = p_hub_dev;
	} else {
		usbh_hub_tail_ptr->nxt_ptr = p_hub_dev;
		usbh_hub_tail_ptr = p_hub_dev;
	}
	irq_unlock(key);
	k_sem_give(&usbh_hub_event_sem);
}

/*
 * Handle device state change notification for hub class devices.
 */
void usbh_hub_class_notify(void *p_class_dev, uint8_t state, void *p_ctx)
{
	struct usbh_hub_dev *p_hub_dev;
	struct usbh_dev *p_dev;

	p_hub_dev = (struct usbh_hub_dev *)p_class_dev;
	p_dev = p_hub_dev->dev_ptr;

	if (p_dev->is_root_hub == true) {
		/* If RH, return immediately. */
		return;
	}

	switch (state) {
	/* External hub has been identified. */
	case USBH_CLASS_DEV_STATE_CONN:
		LOG_WRN("Ext HUB (Addr# %i) connected\r\n", p_dev->dev_addr);
		break;

	case USBH_CLASS_DEV_STATE_DISCONN:
		LOG_WRN("Ext HUB (Addr# %i) disconnected\r\n", p_dev->dev_addr);
		break;

	default:
		break;
	}
}

/*
 * Parse hub descriptor into hub descriptor structure.
 */
void usbh_hub_parse_hub_desc(struct usbh_hub_desc *p_hub_desc, void *p_buf_src)
{
	struct usbh_hub_desc *p_buf_src_desc;
	uint8_t i;

	p_buf_src_desc = (struct usbh_hub_desc *)p_buf_src;

	p_hub_desc->b_desc_length = p_buf_src_desc->b_desc_length;
	p_hub_desc->b_desc_type = p_buf_src_desc->b_desc_type;
	p_hub_desc->b_nbr_ports = p_buf_src_desc->b_nbr_ports;
	p_hub_desc->w_hub_characteristics =
		sys_get_le16((uint8_t *)&p_buf_src_desc->w_hub_characteristics);
	p_hub_desc->b_pwr_on_to_pwr_good = p_buf_src_desc->b_pwr_on_to_pwr_good;
	p_hub_desc->b_hub_contr_current = p_buf_src_desc->b_hub_contr_current;
	p_hub_desc->device_removable = p_buf_src_desc->device_removable;

	for (i = 0; i < USBH_CFG_MAX_HUB_PORTS; i++) {
		p_hub_desc->port_pwr_ctrl_mask[i] = sys_get_le32(
			(uint8_t *)&p_buf_src_desc->port_pwr_ctrl_mask[i]);
	}
}

/*
 * Format hub descriptor from hub descriptor structure.
 */
void usbh_hub_fmt_hub_desc(struct usbh_hub_desc *p_hub_desc, void *p_buf_dest)
{
	struct usbh_hub_desc *p_buf_dest_desc;

	p_buf_dest_desc = (struct usbh_hub_desc *)p_buf_dest;

	p_buf_dest_desc->b_desc_length = p_hub_desc->b_desc_length;
	p_buf_dest_desc->b_desc_type = p_hub_desc->b_desc_type;
	p_buf_dest_desc->b_nbr_ports = p_hub_desc->b_nbr_ports;
	p_buf_dest_desc->w_hub_characteristics =
		sys_get_le16((uint8_t *)&p_hub_desc->w_hub_characteristics);
	p_buf_dest_desc->b_hub_contr_current = p_hub_desc->b_hub_contr_current;
	p_buf_dest_desc->device_removable = p_hub_desc->device_removable;

	memcpy(&p_buf_dest_desc->b_pwr_on_to_pwr_good,
	       &p_hub_desc->b_pwr_on_to_pwr_good, sizeof(uint8_t));
	memcpy(&p_buf_dest_desc->port_pwr_ctrl_mask[0],
	       &p_hub_desc->port_pwr_ctrl_mask[0],
	       (sizeof(uint32_t) * USBH_CFG_MAX_HUB_PORTS));
}
