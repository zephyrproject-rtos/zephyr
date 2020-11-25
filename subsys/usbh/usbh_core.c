/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file is based on usbh_core.c from uC/Modbus Stack.
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

#include <usbh_core.h>
#include <usbh_class.h>
#include <usbh_hub.h>
#include <sys/byteorder.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(usbh_core, CONFIG_USBH_LOG_LEVEL);

K_THREAD_STACK_DEFINE(usbh_async_task_stack, 1024);
K_THREAD_STACK_DEFINE(usbh_hub_event_task_stack, 2048);
K_MEM_POOL_DEFINE(async_urb_ppool, sizeof(struct usbh_urb),
		  sizeof(struct usbh_urb),
		  (USBH_CFG_MAX_NBR_DEVS * USBH_CFG_MAX_EXTRA_URB_PER_DEV),
		  sizeof(uint32_t));

static volatile struct usbh_urb *usbh_urb_head_ptr;
static volatile struct usbh_urb *usbh_urb_tail_ptr;
static struct k_sem usbh_urb_sem;

static struct usbh_host usbh_host;

static int usbh_ep_open(struct usbh_dev *p_dev, struct usbh_if *p_if,
			uint8_t ep_type, uint8_t ep_dir, struct usbh_ep *p_ep);

static uint32_t usbh_sync_transfer(struct usbh_ep *p_ep, void *p_buf,
				   uint32_t buf_len,
				   struct usbh_isoc_desc *p_isoc_desc,
				   uint8_t token, uint32_t timeout_ms,
				   int *p_err);

static int usbh_async_transfer(struct usbh_ep *p_ep, void *p_buf,
			       uint32_t buf_len,
			       struct usbh_isoc_desc *p_isoc_desc,
			       uint8_t token, void *p_fnct, void *p_fnct_arg);

static uint16_t usbh_sync_ctrl_transfer(struct usbh_ep *p_ep, uint8_t b_req,
					uint8_t bm_req_type, uint16_t w_val,
					uint16_t w_ix, void *p_arg,
					uint16_t w_len, uint32_t timeout_ms,
					int *p_err);

static void usbh_urb_abort(struct usbh_urb *p_urb);

static void usb_urb_notify(struct usbh_urb *p_urb);

static int usbh_urb_submit(struct usbh_urb *p_urb);

static void usbh_urb_clr(struct usbh_urb *p_urb);

static int usbh_dflt_ep_open(struct usbh_dev *p_dev);

static int usbh_dev_desc_rd(struct usbh_dev *p_dev);

static int usbh_cfg_rd(struct usbh_dev *p_dev, uint8_t cfg_ix);

static int usbh_cfg_parse(struct usbh_dev *p_dev, struct usbh_cfg *p_cfg);

static int usbh_dev_addr_set(struct usbh_dev *p_dev);

static uint32_t usbh_str_desc_get(struct usbh_dev *p_dev, uint8_t desc_ix,
				  uint16_t lang_id, void *p_buf,
				  uint32_t buf_len, int *p_err);

static void usbh_str_desc_print(struct usbh_dev *p_dev, uint8_t *p_str_prefix,
				uint8_t desc_idx);

static struct usbh_desc_hdr *usbh_next_desc_get(void *p_buf,
						uint32_t *p_offset);

static void usbh_fmt_setup_req(struct usbh_setup_req *p_setup_req,
			       void *p_buf_dest);

static void usbh_parse_dev_desc(struct usbh_dev_desc *p_dev_desc,
				void *p_buf_src);

static void usbh_parse_cfg_desc(struct usbh_cfg_desc *p_cfg_desc,
				void *p_buf_src);

static void usbh_parse_if_desc(struct usbh_if_desc *p_if_desc, void *p_buf_src);

static void usbh_parse_ep_desc(struct usbh_ep_desc *p_ep_desc, void *p_buf_src);

static void usbh_async_task(void *p_arg, void *p_arg2, void *p_arg3);

/*
 * Allocates and initializes resources required by USB Host stack.
 */
int usbh_init(void)
{
	int err;
	uint8_t ix;

	usbh_urb_head_ptr = NULL;
	usbh_urb_tail_ptr = NULL;

	usbh_host.hc_nbr_next = 0;
	usbh_host.state = USBH_HOST_STATE_NONE;

	for (ix = 0; ix < USBH_CFG_MAX_NBR_CLASS_DRVS; ix++) {
		/* Clr class drv struct table.*/
		usbh_class_drv_list[ix].class_drv_ptr = NULL;
		usbh_class_drv_list[ix].notify_fnct_ptr = NULL;
		usbh_class_drv_list[ix].notify_arg_ptr = NULL;
		usbh_class_drv_list[ix].in_use = 0;
	}

	err = usbh_reg_class_drv(&usbh_hub_drv, usbh_hub_class_notify, NULL);
	if (err != 0) {
		return err;
	}

	err = k_sem_init(&usbh_urb_sem, 0, USBH_OS_SEM_REQUIRED);
	if (err != 0) {
		return err;
	}

	/* Create a task for processing async req.*/
	k_thread_create(&usbh_host.h_async_task, usbh_async_task_stack,
			K_THREAD_STACK_SIZEOF(usbh_async_task_stack),
			usbh_async_task, NULL, NULL, NULL, 0, 0, K_NO_WAIT);

	/* Create a task for processing hub events.*/
	k_thread_create(&usbh_host.h_hub_task, usbh_hub_event_task_stack,
			K_THREAD_STACK_SIZEOF(usbh_hub_event_task_stack),
			usbh_hub_event_task, NULL, NULL, NULL, 0, 0, K_NO_WAIT);

	for (ix = 0; ix < USBH_MAX_NBR_DEVS; ix++) {
		/* init USB dev list.*/
		/* USB addr is ix + 1. Addr 0 is rsvd.*/
		usbh_host.dev_list[ix].dev_addr = ix + 1;
		k_mutex_init(&usbh_host.dev_list[ix].dflt_ep_mutex);
	}
	usbh_host.isoc_cnt = (USBH_CFG_MAX_ISOC_DESC - 1);
	usbh_host.dev_cnt = (USBH_MAX_NBR_DEVS - 1);
	usbh_host.async_urb_pool = async_urb_ppool;

	return 0;
}

/*
 * Suspends USB Host Stack by calling suspend for every class driver loaded
 * and then calling the host controller suspend.
 */
int usbh_suspend(void)
{
	uint8_t ix;
	struct usbh_hc *hc;
	int err;

	for (ix = 0; ix < usbh_host.hc_nbr_next; ix++) {
		hc = &usbh_host.hc_tbl[ix];

		/* Suspend RH, and all downstream dev.*/
		usbh_class_suspend(hc->hc_drv.rh_dev_ptr);
		k_mutex_lock(&hc->hcd_mutex, K_NO_WAIT);
		hc->hc_drv.api_ptr->suspend(&hc->hc_drv, &err);
		/* Suspend HC.*/
		k_mutex_unlock(&hc->hcd_mutex);
	}

	usbh_host.state = USBH_HOST_STATE_SUSPENDED;

	return err;
}

/*
 * Resumes USB Host Stack by calling host controller resume and then
 * calling resume for every class driver loaded.
 */
int usbh_resume(void)
{
	uint8_t ix;
	struct usbh_hc *hc;
	int err;

	for (ix = 0; ix < usbh_host.hc_nbr_next; ix++) {
		hc = &usbh_host.hc_tbl[ix];

		k_mutex_lock(&hc->hcd_mutex, K_NO_WAIT);
		/* Resume HC.*/
		hc->hc_drv.api_ptr->resume(&hc->hc_drv, &err);
		k_mutex_unlock(&hc->hcd_mutex);

		/* Resume RH, and all downstream dev.*/
		usbh_class_resume(hc->hc_drv.rh_dev_ptr);
	}

	usbh_host.state = USBH_HOST_STATE_RESUMED;

	return err;
}

/*
 * Add a host controller.
 */
uint8_t usbh_hc_add(int *p_err)
{
	struct usbh_dev *p_rh_dev;
	uint8_t hc_nbr;
	struct usbh_hc *p_hc;
	struct usbh_hc_drv *p_hc_drv;
	int key;

	key = irq_lock();
	hc_nbr = usbh_host.hc_nbr_next;
	if (hc_nbr >= USBH_CFG_MAX_NBR_HC) {
		/* Chk if HC nbr is valid.*/
		irq_unlock(key);
		*p_err = -EIO;
		return USBH_HC_NBR_NONE;
	}
	usbh_host.hc_nbr_next++;
	irq_unlock(key);

	p_hc = &usbh_host.hc_tbl[hc_nbr];
	p_hc_drv = &p_hc->hc_drv;

	if (usbh_host.dev_cnt < 0) {
		return USBH_HC_NBR_NONE;
	}
	p_rh_dev = &usbh_host.dev_list[usbh_host.dev_cnt--];

	p_rh_dev->is_root_hub = true;
	p_rh_dev->hc_ptr = p_hc;

	p_hc->host_ptr = &usbh_host;
	p_hc->is_vir_rh = true;

	p_hc_drv->data_ptr = NULL;
	p_hc_drv->rh_dev_ptr = p_rh_dev;
	p_hc_drv->api_ptr = &usbh_hcd_api;
	p_hc_drv->rh_api_ptr = &usbh_hcd_rh_api;
	p_hc_drv->nbr = hc_nbr;

	k_mutex_init(&p_hc->hcd_mutex);

	k_mutex_lock(&p_hc->hcd_mutex, K_NO_WAIT);
	/* init HCD.*/
	p_hc->hc_drv.api_ptr->init(&p_hc->hc_drv, p_err);
	k_mutex_unlock(&p_hc->hcd_mutex);
	if (*p_err != 0) {
		return USBH_HC_NBR_NONE;
	}

	k_mutex_lock(&p_hc->hcd_mutex, K_NO_WAIT);
	p_rh_dev->dev_spd = p_hc->hc_drv.api_ptr->spd_get(&p_hc->hc_drv, p_err);
	k_mutex_unlock(&p_hc->hcd_mutex);

	return hc_nbr;
}

/*
 * Start given host controller.
 */
int usbh_hc_start(uint8_t hc_nbr)
{
	LOG_DBG("start host controller");
	int err;
	struct usbh_hc *p_hc;
	struct usbh_dev *p_rh_dev;

	if (hc_nbr >= usbh_host.hc_nbr_next) {
		/* Chk if HC nbr is valid. */
		LOG_DBG("host controller number invalid.");
		return -EINVAL;
	}

	p_hc = &usbh_host.hc_tbl[hc_nbr];
	p_rh_dev = p_hc->hc_drv.rh_dev_ptr;
	/* Add RH of given HC. */
	err = usbh_dev_conn(p_rh_dev);
	if (err == 0) {
		usbh_host.state = USBH_HOST_STATE_RESUMED;
	} else {
		LOG_DBG("device disconnected");
		usbh_dev_disconn(p_rh_dev);
	}
	k_mutex_lock(&p_hc->hcd_mutex, K_NO_WAIT);
	p_hc->hc_drv.api_ptr->start(&p_hc->hc_drv, &err);

	k_mutex_unlock(&p_hc->hcd_mutex);

	return err;
}

/*
 * Stop the given host controller.
 */
int usbh_hc_stop(uint8_t hc_nbr)
{
	int err;
	struct usbh_hc *p_hc;
	struct usbh_dev *p_rh_dev;

	if (hc_nbr >= usbh_host.hc_nbr_next) {
		/* Chk if HC nbr is valid.*/
		return -EINVAL;
	}

	p_hc = &usbh_host.hc_tbl[hc_nbr];
	p_rh_dev = p_hc->hc_drv.rh_dev_ptr;
	/* Disconn RH dev.*/
	usbh_dev_disconn(p_rh_dev);
	k_mutex_lock(&p_hc->hcd_mutex, K_NO_WAIT);
	p_hc->hc_drv.api_ptr->stop(&p_hc->hc_drv, &err);

	k_mutex_unlock(&p_hc->hcd_mutex);

	return err;
}

/*
 * Enable given port of given host controller's root hub.
 */
int usbh_hc_port_en(uint8_t hc_nbr, uint8_t port_nbr)
{
	LOG_DBG("enable port");
	int err;
	struct usbh_hc *p_hc;

	if (hc_nbr >= usbh_host.hc_nbr_next) {
		/* Chk if HC nbr is valid.*/
		return -EINVAL;
	}

	p_hc = &usbh_host.hc_tbl[hc_nbr];
	err = usbh_hub_port_en(p_hc->rh_class_dev_ptr, port_nbr);

	return err;
}

/*
 * Disable given port of given host controller's root hub.
 */
int usbh_hc_port_dis(uint8_t hc_nbr, uint8_t port_nbr)
{
	LOG_DBG("disable port");
	int err;
	struct usbh_hc *p_hc;

	if (hc_nbr >= usbh_host.hc_nbr_next) {
		/* Chk if HC nbr is valid.*/
		return -EINVAL;
	}

	p_hc = &usbh_host.hc_tbl[hc_nbr];
	err = usbh_hub_port_dis(p_hc->rh_class_dev_ptr, port_nbr);
	return err;
}

/*
 * Get current frame number.
 */
uint32_t usbh_hc_frame_nbr_get(uint8_t hc_nbr, int *p_err)
{
	uint32_t frame_nbr;
	struct usbh_hc *p_hc;

	if (hc_nbr >= usbh_host.hc_nbr_next) {
		/* Chk if HC nbr is valid.*/
		*p_err = -EINVAL;
		return 0;
	}

	p_hc = &usbh_host.hc_tbl[hc_nbr];

	k_mutex_lock(&p_hc->hcd_mutex, K_NO_WAIT);
	frame_nbr = p_hc->hc_drv.api_ptr->frm_nbr_get(&p_hc->hc_drv, p_err);

	k_mutex_unlock(&p_hc->hcd_mutex);

	return frame_nbr;
}

/*
 * Enumerates newly connected USB device. Reads device and
 * configuration descriptor from device and loads appropriate class driver(s).
 */
int usbh_dev_conn(struct usbh_dev *p_dev)
{
	LOG_DBG("device connected");
	int err;
	uint8_t nbr_cfgs;
	uint8_t cfg_ix;

	p_dev->sel_cfg = 0;

	p_dev->class_drv_reg_ptr = NULL;
	memset(p_dev->dev_desc, 0, USBH_LEN_DESC_DEV);

	err = usbh_dflt_ep_open(p_dev);
	if (err != 0) {
		return err;
	}
	/*  RD DEV DESC  */
	err = usbh_dev_desc_rd(p_dev);
	if (err != 0) {
		return err;
	}
	/*  ASSIGN NEW ADDR TO DEV  */
	err = usbh_dev_addr_set(p_dev);
	if (err != 0) {
		return err;
	}

	LOG_DBG("Port %d: Device Address: %d.\r\n", p_dev->port_nbr,
		p_dev->dev_addr);

	if (p_dev->dev_desc[14] != 0u) {
		usbh_str_desc_print(p_dev, (uint8_t *)"Manufacturer: ",
				    p_dev->dev_desc[14]);
	}

	if (p_dev->dev_desc[15] != 0u) {
		usbh_str_desc_print(
			p_dev, (uint8_t *)"Product: ", p_dev->dev_desc[15]);
	}
	/*  GET NBR OF CFG PRESENT IN DEV  */
	nbr_cfgs = usbh_dev_cfg_nbr_get(p_dev);
	if (nbr_cfgs == 0) {
		return -EAGAIN;
	} else if (nbr_cfgs > USBH_CFG_MAX_NBR_CFGS) {
		return -EAGAIN;
	}
	/*  RD ALL CFG  */
	for (cfg_ix = 0; cfg_ix < nbr_cfgs; cfg_ix++) {
		err = usbh_cfg_rd(p_dev, cfg_ix);
		if (err != 0) {
			return err;
		}
	}
	/*  PROBE/LOAD CLASS DRV(S)  */
	err = usbh_class_drv_conn(p_dev);

	return err;
}

/*
 * Unload class drivers & close default endpoint.
 */
void usbh_dev_disconn(struct usbh_dev *p_dev)
{
	LOG_DBG("device disconnected");
	/* Unload class drv(s).*/
	usbh_class_drv_disconn(p_dev);
	/* Close dflt EPs.*/
	usbh_ep_close(&p_dev->dflt_ep);
}

/*
 * Get number of configurations supported by specified device.
 */
uint8_t usbh_dev_cfg_nbr_get(struct usbh_dev *p_dev)
{
	return p_dev->dev_desc[17];
}

/*
 * Get device descriptor of specified USB device.
 */
void usbh_dev_desc_get(struct usbh_dev *p_dev, struct usbh_dev_desc *p_dev_desc)
{
	usbh_parse_dev_desc(p_dev_desc, (void *)p_dev->dev_desc);
}

/*
 * Select a configration in specified device.
 */
int usbh_cfg_set(struct usbh_dev *p_dev, uint8_t cfg_nbr)
{
	int err;

	usbh_ctrl_tx(p_dev, USBH_REQ_SET_CFG,
		     (USBH_REQ_DIR_HOST_TO_DEV | USBH_REQ_RECIPIENT_DEV),
		     cfg_nbr, 0, NULL, 0, USBH_CFG_STD_REQ_TIMEOUT, &err);

	if (err == 0) {
		p_dev->sel_cfg = cfg_nbr;
	}

	return err;
}

/*
 * Get a pointer to specified configuration data of specified device.
 */
struct usbh_cfg *usbh_cfg_get(struct usbh_dev *p_dev, uint8_t cfg_ix)
{
	uint8_t nbr_cfgs;

	if (p_dev == NULL) {
		return NULL;
	}
	/* Get nbr of cfg(s) present in dev.*/
	nbr_cfgs = usbh_dev_cfg_nbr_get(p_dev);
	if ((cfg_ix >= nbr_cfgs) || (nbr_cfgs == 0)) {
		return NULL;
	}
	/* Get cfg struct.*/
	return (&p_dev->cfg_list[cfg_ix]);
}

/*
 * Get number of interfaces in given configuration.
 */
uint8_t usbh_cfg_if_nbr_get(struct usbh_cfg *p_cfg)
{
	if (p_cfg != NULL) {
		return p_cfg->cfg_data[4];
	} else {
		return 0;
	}
}

/*
 * Get configuration descriptor data.
 */
int usbh_cfg_desc_get(struct usbh_cfg *p_cfg, struct usbh_cfg_desc *p_cfg_desc)
{
	struct usbh_desc_hdr *p_desc;

	if ((p_cfg == NULL) || (p_cfg_desc == NULL)) {
		return -EINVAL;
	}
	/* Check for valid cfg desc.*/
	p_desc = (struct usbh_desc_hdr *)p_cfg->cfg_data;

	if ((p_desc->b_length == USBH_LEN_DESC_CFG) &&
	    (p_desc->b_desc_type == USBH_DESC_TYPE_CFG)) {
		usbh_parse_cfg_desc(p_cfg_desc, (void *)p_desc);

		return 0;
	} else {
		return -EAGAIN;
	}
}

/*
 * Get extra descriptor immediately following configuration descriptor.
 */
struct usbh_desc_hdr *usbh_cfg_extra_desc_get(struct usbh_cfg *p_cfg,
					      int *p_err)
{
	struct usbh_desc_hdr *p_desc;
	struct usbh_desc_hdr *p_extra_desc;
	uint32_t cfg_off;

	if (p_cfg == NULL) {
		*p_err = -EINVAL;
		return NULL;
	}
	/* Get config desc data.*/
	p_desc = (struct usbh_desc_hdr *)p_cfg->cfg_data;

	if ((p_desc->b_length == USBH_LEN_DESC_CFG) &&
	    (p_desc->b_desc_type == USBH_DESC_TYPE_CFG) &&
	    (p_cfg->cfg_data_len > (p_desc->b_length + 2))) {
		cfg_off = p_desc->b_length;
		/* Get desc that follows config desc.*/
		p_extra_desc = usbh_next_desc_get(p_desc, &cfg_off);

		/* No extra desc present.*/
		if (p_extra_desc->b_desc_type != USBH_DESC_TYPE_IF) {
			*p_err = 0;
			return p_extra_desc;
		}
	}

	*p_err = -ENOENT;

	return NULL;
}

/*
 * Select specified alternate setting of interface.
 */
int usbh_if_set(struct usbh_if *p_if, uint8_t alt_nbr)
{
	uint8_t nbr_alts;
	uint8_t if_nbr;
	struct usbh_dev *p_dev;
	int err;

	if (p_if == NULL) {
		return -EINVAL;
	}
	/* Get nbr of alternate settings in IF.*/
	nbr_alts = usbh_if_alt_nbr_get(p_if);
	if (alt_nbr >= nbr_alts) {
		return -EINVAL;
	}
	/* Get IF nbr.*/
	if_nbr = usbh_if_nbr_get(p_if);
	p_dev = p_if->dev_ptr;

	usbh_ctrl_tx(p_dev, USBH_REQ_SET_IF,
		     (USBH_REQ_DIR_HOST_TO_DEV | USBH_REQ_RECIPIENT_IF),
		     alt_nbr, if_nbr, NULL, 0, USBH_CFG_STD_REQ_TIMEOUT, &err);
	if (err != 0) {
		return err;
	}
	/* Update selected alternate setting.*/
	p_if->alt_ix_sel = alt_nbr;

	return err;
}

/*
 * Get specified interface from given configuration.
 */
struct usbh_if *usbu_if_get(struct usbh_cfg *p_cfg, uint8_t if_ix)
{
	uint8_t nbr_ifs;

	/* Get nbr of IFs.*/
	nbr_ifs = usbh_cfg_if_nbr_get(p_cfg);

	if ((if_ix < nbr_ifs) && (if_ix < USBH_CFG_MAX_NBR_IFS)) {
		/* Return IF structure at selected ix.*/
		return &p_cfg->if_list[if_ix];
	} else {
		return NULL;
	}
}

/*
 * Get number of alternate settings supported by the given interface.
 */
uint8_t usbh_if_alt_nbr_get(struct usbh_if *p_if)
{
	struct usbh_desc_hdr *p_desc;
	uint32_t if_off;
	uint8_t nbr_alts;

	nbr_alts = 0;
	if_off = 0;
	p_desc = (struct usbh_desc_hdr *)p_if->if_data_ptr;

	while (if_off < p_if->if_data_len) {
		/* Cnt nbr of alternate settings.*/
		p_desc = usbh_next_desc_get((void *)p_desc, &if_off);

		if (p_desc->b_desc_type == USBH_DESC_TYPE_IF) {
			nbr_alts++;
		}
	}

	return nbr_alts;
}

/*
 * Get number of given interface.
 */
uint8_t usbh_if_nbr_get(struct usbh_if *p_if)
{
	return p_if->if_data_ptr[2];
}

/*
 * Determine number of endpoints in given alternate setting of interface.
 */
uint8_t usbh_if_ep_nbr_get(struct usbh_if *p_if, uint8_t alt_ix)
{
	struct usbh_desc_hdr *p_desc;
	uint32_t if_off;

	if_off = 0;
	p_desc = (struct usbh_desc_hdr *)p_if->if_data_ptr;

	while (if_off < p_if->if_data_len) {
		p_desc = usbh_next_desc_get((void *)p_desc, &if_off);
		/* IF desc.*/
		if (p_desc->b_desc_type == USBH_DESC_TYPE_IF) {
			if (alt_ix == ((uint8_t *)p_desc)[3]) {
				/* Chk alternate setting.*/
				/* IF desc offset 4 contains nbr of EPs.*/
				return (((uint8_t *)p_desc)[4]);
			}
		}
	}

	return 0;
}

/*
 * Get descriptor of interface at specified alternate setting index.
 */
int usbh_if_desc_get(struct usbh_if *p_if, uint8_t alt_ix,
		     struct usbh_if_desc *p_if_desc)
{
	struct usbh_desc_hdr *p_desc;
	uint32_t if_off;

	if_off = 0;
	p_desc = (struct usbh_desc_hdr *)p_if->if_data_ptr;

	while (if_off < p_if->if_data_len) {
		p_desc = usbh_next_desc_get((void *)p_desc, &if_off);

		if ((p_desc->b_length == USBH_LEN_DESC_IF) &&
		    (p_desc->b_desc_type == USBH_DESC_TYPE_IF) &&
		    (alt_ix == ((uint8_t *)p_desc)[3])) {
			usbh_parse_if_desc(p_if_desc, (void *)p_desc);
			return 0;
		}
	}

	return -EINVAL;
}

/*
 * Get the descriptor immediately following the interface descriptor.
 */
uint8_t *usbh_if_extra_desc_get(struct usbh_if *p_if, uint8_t alt_ix,
				uint16_t *p_data_len)
{
	struct usbh_desc_hdr *p_desc;
	uint8_t *p_data;
	uint32_t if_off;

	if ((p_if == NULL) || (p_if->if_data_ptr == NULL)) {
		return NULL;
	}

	if_off = 0;
	p_desc = (struct usbh_desc_hdr *)p_if->if_data_ptr;

	while (if_off < p_if->if_data_len) {
		/* Get next desc from IF.*/
		p_desc = usbh_next_desc_get((void *)p_desc, &if_off);

		if ((p_desc->b_length == USBH_LEN_DESC_IF) &&
		    (p_desc->b_desc_type == USBH_DESC_TYPE_IF) &&
		    (alt_ix == ((uint8_t *)p_desc)[3])) {
			if (if_off < p_if->if_data_len) {
				/*
				 * Get desc that follows selected
				 * alternate setting.
				 */
				p_desc = usbh_next_desc_get((void *)p_desc,
							    &if_off);
				p_data = (uint8_t *)p_desc;
				*p_data_len = 0;

				while ((p_desc->b_desc_type !=
					USBH_DESC_TYPE_IF) &&
				       (p_desc->b_desc_type !=
					USBH_DESC_TYPE_EP)) {
					*p_data_len += p_desc->b_length;
					/* Get next desc from IF.*/
					p_desc = usbh_next_desc_get(
						(void *)p_desc, &if_off);
					if (if_off >= p_if->if_data_len) {
						break;
					}
				}

				if (*p_data_len == 0) {
					return NULL;
				} else {
					return (uint8_t *)p_data;
				}
			}
		}
	}

	return NULL;
}

/*
 * Open a bulk IN endpoint.
 */
int usbh_bulk_in_open(struct usbh_dev *p_dev, struct usbh_if *p_if,
		      struct usbh_ep *p_ep)
{
	int err;

	err = usbh_ep_open(p_dev, p_if, USBH_EP_TYPE_BULK, USBH_EP_DIR_IN,
			   p_ep);

	return err;
}

/*
 * Open a bulk OUT endpoint.
 */
int usbh_bulk_out_open(struct usbh_dev *p_dev, struct usbh_if *p_if,
		       struct usbh_ep *p_ep)
{
	int err;

	err = usbh_ep_open(p_dev, p_if, USBH_EP_TYPE_BULK, USBH_EP_DIR_OUT,
			   p_ep);

	return err;
}

/*
 * Open an interrupt IN endpoint.
 */
int usbh_intr_in_open(struct usbh_dev *p_dev, struct usbh_if *p_if,
		      struct usbh_ep *p_ep)
{
	int err;

	err = usbh_ep_open(p_dev, p_if, USBH_EP_TYPE_INTR, USBH_EP_DIR_IN,
			   p_ep);

	return err;
}

/*
 * Open and interrupt OUT endpoint.
 */
int usbh_intr_out_open(struct usbh_dev *p_dev, struct usbh_if *p_if,
		       struct usbh_ep *p_ep)
{
	int err;

	err = usbh_ep_open(p_dev, p_if, USBH_EP_TYPE_INTR, USBH_EP_DIR_OUT,
			   p_ep);

	return err;
}

/*
 * Open an isochronous IN endpoint.
 */
int usbh_isoc_in_open(struct usbh_dev *p_dev, struct usbh_if *p_if,
		      struct usbh_ep *p_ep)
{
	int err;

	err = usbh_ep_open(p_dev, p_if, USBH_EP_TYPE_ISOC, USBH_EP_DIR_IN,
			   p_ep);

	return err;
}

/*
 * Open an isochronous OUT endpoint.
 */
int usbh_isoc_out_open(struct usbh_dev *p_dev, struct usbh_if *p_if,
		       struct usbh_ep *p_ep)
{
	int err;

	err = usbh_ep_open(p_dev, p_if, USBH_EP_TYPE_ISOC, USBH_EP_DIR_OUT,
			   p_ep);

	return err;
}

/*
 * Issue control request to device and send data to it.
 */
uint16_t usbh_ctrl_tx(struct usbh_dev *p_dev, uint8_t b_req,
		      uint8_t bm_req_type, uint16_t w_val, uint16_t w_ix,
		      void *p_data, uint16_t w_len, uint32_t timeout_ms,
		      int *p_err)
{
	uint16_t xfer_len;

	k_mutex_lock(&p_dev->dflt_ep_mutex, K_NO_WAIT);
	/* Check if RH features are supported.*/
	if ((p_dev->is_root_hub == true) &&
	    (p_dev->hc_ptr->is_vir_rh == true)) {
		/* Send req to virtual HUB.*/
		xfer_len = usbh_rh_ctrl_req(p_dev->hc_ptr, b_req, bm_req_type,
					    w_val, w_ix, p_data, w_len, p_err);
	} else {
		xfer_len = usbh_sync_ctrl_transfer(&p_dev->dflt_ep, b_req,
						   bm_req_type, w_val, w_ix,
						   p_data, w_len, timeout_ms,
						   p_err);
	}

	k_mutex_unlock(&p_dev->dflt_ep_mutex);
	return xfer_len;
}

/*
 * Issue control request to device and receive data from it.
 */
uint16_t usbh_ctrl_rx(struct usbh_dev *p_dev, uint8_t b_req,
		      uint8_t bm_req_type, uint16_t w_val, uint16_t w_ix,
		      void *p_data, uint16_t w_len, uint32_t timeout_ms,
		      int *p_err)
{
	uint16_t xfer_len;

	k_mutex_lock(&p_dev->dflt_ep_mutex, K_NO_WAIT);
	/* Check if RH features are supported.*/
	if ((p_dev->is_root_hub == true) &&
	    (p_dev->hc_ptr->is_vir_rh == true)) {
		/* Send req to virtual HUB.*/
		xfer_len = usbh_rh_ctrl_req(p_dev->hc_ptr, b_req, bm_req_type,
					    w_val, w_ix, p_data, w_len, p_err);
	} else {
		xfer_len = usbh_sync_ctrl_transfer(&p_dev->dflt_ep, b_req,
						   bm_req_type, w_val, w_ix,
						   p_data, w_len, timeout_ms,
						   p_err);
	}

	k_mutex_unlock(&p_dev->dflt_ep_mutex);

	return xfer_len;
}

/*
 * Issue bulk request to transmit data to device.
 */
uint32_t usbh_bulk_tx(struct usbh_ep *p_ep, void *p_buf, uint32_t buf_len,
		      uint32_t timeout_ms, int *p_err)
{
	uint8_t ep_type;
	uint8_t ep_dir;
	uint32_t xfer_len;

	if (p_ep == NULL) {
		*p_err = -EINVAL;
		return 0;
	}

	ep_type = usbh_ep_type_get(p_ep);
	ep_dir = usbh_ep_dir_get(p_ep);

	if ((ep_type != USBH_EP_TYPE_BULK) || (ep_dir != USBH_EP_DIR_OUT)) {
		*p_err = -EAGAIN;
		return 0;
	}

	xfer_len = usbh_sync_transfer(p_ep, p_buf, buf_len, NULL,
				      USBH_TOKEN_OUT, timeout_ms, p_err);

	return xfer_len;
}

/*
 * Issue asynchronous bulk request to transmit data to device.
 */
int usbh_bulk_tx_async(struct usbh_ep *p_ep, void *p_buf, uint32_t buf_len,
		       usbh_xfer_cmpl_fnct fnct, void *p_fnct_arg)
{
	int err;
	uint8_t ep_type;
	uint8_t ep_dir;

	if (p_ep == NULL) {
		return -EINVAL;
	}

	ep_type = usbh_ep_type_get(p_ep);
	ep_dir = usbh_ep_dir_get(p_ep);

	if ((ep_type != USBH_EP_TYPE_BULK) || (ep_dir != USBH_EP_DIR_OUT)) {
		return -EAGAIN;
	}

	err = usbh_async_transfer(p_ep, p_buf, buf_len, NULL, USBH_TOKEN_OUT,
				  (void *)fnct, p_fnct_arg);

	return err;
}

/*
 * Issue bulk request to receive data from device.
 */
uint32_t usbh_bulk_rx(struct usbh_ep *p_ep, void *p_buf, uint32_t buf_len,
		      uint32_t timeout_ms, int *p_err)
{
	uint8_t ep_type;
	uint8_t ep_dir;
	uint32_t xfer_len;

	if (p_ep == NULL) {
		*p_err = -EINVAL;
		return 0;
	}

	ep_type = usbh_ep_type_get(p_ep);
	ep_dir = usbh_ep_dir_get(p_ep);

	if ((ep_type != USBH_EP_TYPE_BULK) || (ep_dir != USBH_EP_DIR_IN)) {
		*p_err = -EAGAIN;
		return 0;
	}

	xfer_len = usbh_sync_transfer(p_ep, p_buf, buf_len, NULL, USBH_TOKEN_IN,
				      timeout_ms, p_err);

	return xfer_len;
}

/*
 * Issue asynchronous bulk request to receive data from device.
 */
int usbh_bulk_rx_async(struct usbh_ep *p_ep, void *p_buf, uint32_t buf_len,
		       usbh_xfer_cmpl_fnct fnct, void *p_fnct_arg)
{
	int err;
	uint8_t ep_type;
	uint8_t ep_dir;

	if (p_ep == NULL) {
		return -EINVAL;
	}

	ep_type = usbh_ep_type_get(p_ep);
	ep_dir = usbh_ep_dir_get(p_ep);

	if ((ep_type != USBH_EP_TYPE_BULK) || (ep_dir != USBH_EP_DIR_IN)) {
		return -EAGAIN;
	}

	err = usbh_async_transfer(p_ep, p_buf, buf_len, NULL, USBH_TOKEN_IN,
				  (void *)fnct, p_fnct_arg);

	return err;
}

/*
 * Issue interrupt request to transmit data to device
 */
uint32_t usbh_intr_tx(struct usbh_ep *p_ep, void *p_buf, uint32_t buf_len,
		      uint32_t timeout_ms, int *p_err)
{
	uint8_t ep_type;
	uint8_t ep_dir;
	uint32_t xfer_len;

	if (p_ep == NULL) {
		return -EINVAL;
	}

	ep_type = usbh_ep_type_get(p_ep);
	ep_dir = usbh_ep_dir_get(p_ep);

	if ((ep_type != USBH_EP_TYPE_INTR) || (ep_dir != USBH_EP_DIR_OUT)) {
		return -EAGAIN;
	}

	xfer_len = usbh_sync_transfer(p_ep, p_buf, buf_len, NULL,
				      USBH_TOKEN_OUT, timeout_ms, p_err);

	return xfer_len;
}

/*
 * Issue asynchronous interrupt request to transmit data to device.
 */
int usbh_intr_tx_async(struct usbh_ep *p_ep, void *p_buf, uint32_t buf_len,
		       usbh_xfer_cmpl_fnct fnct, void *p_fnct_arg)
{
	int err;
	uint8_t ep_type;
	uint8_t ep_dir;

	if (p_ep == NULL) {
		return -EINVAL;
	}

	ep_type = usbh_ep_type_get(p_ep);
	ep_dir = usbh_ep_dir_get(p_ep);

	if ((ep_type != USBH_EP_TYPE_INTR) || (ep_dir != USBH_EP_DIR_OUT)) {
		return -EAGAIN;
	}

	err = usbh_async_transfer(p_ep, p_buf, buf_len, NULL, USBH_TOKEN_OUT,
				  (void *)fnct, p_fnct_arg);

	return err;
}

/*
 * Issue interrupt request to receive data from device.
 */
uint32_t usbh_intr_rx(struct usbh_ep *p_ep, void *p_buf, uint32_t buf_len,
		      uint32_t timeout_ms, int *p_err)
{
	uint8_t ep_type;
	uint8_t ep_dir;
	uint32_t xfer_len;

	if (p_ep == NULL) {
		return -EINVAL;
	}

	ep_type = usbh_ep_type_get(p_ep);
	ep_dir = usbh_ep_dir_get(p_ep);

	if ((ep_type != USBH_EP_TYPE_INTR) || (ep_dir != USBH_EP_DIR_IN)) {
		return -EAGAIN;
	}

	xfer_len = usbh_sync_transfer(p_ep, p_buf, buf_len, NULL, USBH_TOKEN_IN,
				      timeout_ms, p_err);

	return xfer_len;
}

/*
 * Issue asynchronous interrupt request to receive data from device.
 */
int usbh_intr_rx_async(struct usbh_ep *p_ep, void *p_buf, uint32_t buf_len,
		       usbh_xfer_cmpl_fnct fnct, void *p_fnct_arg)
{
	int err;
	uint8_t ep_type;
	uint8_t ep_dir;

	/* Argument checks for valid settings*/
	if (p_ep == NULL) {
		return -EINVAL;
	}

	if (p_ep->is_open == false) {
		return -EAGAIN;
	}

	ep_type = usbh_ep_type_get(p_ep);
	ep_dir = usbh_ep_dir_get(p_ep);

	if ((ep_type != USBH_EP_TYPE_INTR) || (ep_dir != USBH_EP_DIR_IN)) {
		return -EAGAIN;
	}

	err = usbh_async_transfer(p_ep, p_buf, buf_len, NULL, USBH_TOKEN_IN,
				  (void *)fnct, p_fnct_arg);

	return err;
}

/*
 * Issue isochronous request to transmit data to device.
 */
uint32_t usbh_isoc_tx(struct usbh_ep *p_ep, uint8_t *p_buf, uint32_t buf_len,
		      uint32_t start_frm, uint32_t nbr_frm, uint16_t *p_frm_len,
		      int *p_frm_err, uint32_t timeout_ms, int *p_err)
{
	uint8_t ep_type;
	uint8_t ep_dir;
	uint32_t xfer_len;
	struct usbh_isoc_desc isoc_desc;

	if (p_ep == NULL) {
		return -EINVAL;
	}

	ep_type = usbh_ep_type_get(p_ep);
	ep_dir = usbh_ep_dir_get(p_ep);

	if ((ep_type != USBH_EP_TYPE_ISOC) || (ep_dir != USBH_EP_DIR_OUT)) {
		return -EAGAIN;
	}

	isoc_desc.buf_ptr = p_buf;
	isoc_desc.buf_len = buf_len;
	isoc_desc.start_frm = start_frm;
	isoc_desc.nbr_frm = nbr_frm;
	isoc_desc.frm_len = p_frm_len;
	isoc_desc.frm_err = p_frm_err;

	xfer_len = usbh_sync_transfer(p_ep, p_buf, buf_len, &isoc_desc,
				      USBH_TOKEN_OUT, timeout_ms, p_err);

	return xfer_len;
}

/*
 * Issue asynchronous isochronous request to transmit data to device.
 */
int usbh_isoc_tx_async(struct usbh_ep *p_ep, uint8_t *p_buf, uint32_t buf_len,
		       uint32_t start_frm, uint32_t nbr_frm,
		       uint16_t *p_frm_len, int *p_frm_err,
		       usbh_isoc_cmpl_fnct fnct, void *p_fnct_arg)
{
	int err;
	uint8_t ep_type;
	uint8_t ep_dir;
	struct usbh_isoc_desc *p_isoc_desc;

	if (p_ep == NULL) {
		return -EINVAL;
	}

	ep_type = usbh_ep_type_get(p_ep);
	ep_dir = usbh_ep_dir_get(p_ep);

	if ((ep_type != USBH_EP_TYPE_ISOC) || (ep_dir != USBH_EP_DIR_OUT)) {
		return -EAGAIN;
	}

	if (p_ep->dev_ptr->hc_ptr->host_ptr->isoc_cnt < 0) {
		return -ENOMEM;
	}
	p_isoc_desc =
		&p_ep->dev_ptr->hc_ptr->host_ptr
		->isoc_desc[p_ep->dev_ptr->hc_ptr->host_ptr->isoc_cnt--];

	p_isoc_desc->buf_ptr = p_buf;
	p_isoc_desc->buf_len = buf_len;
	p_isoc_desc->start_frm = start_frm;
	p_isoc_desc->nbr_frm = nbr_frm;
	p_isoc_desc->frm_len = p_frm_len;
	p_isoc_desc->frm_err = p_frm_err;

	err = usbh_async_transfer(p_ep, p_buf, buf_len, p_isoc_desc,
				  USBH_TOKEN_IN, (void *)fnct, p_fnct_arg);
	if (err != 0) {
		p_ep->dev_ptr->hc_ptr->host_ptr->isoc_cnt++;
	}

	return err;
}

/*
 * Issue isochronous request to receive data from device.
 */
uint32_t usbh_isoc_rx(struct usbh_ep *p_ep, uint8_t *p_buf, uint32_t buf_len,
		      uint32_t start_frm, uint32_t nbr_frm, uint16_t *p_frm_len,
		      int *p_frm_err, uint32_t timeout_ms, int *p_err)
{
	uint8_t ep_type;
	uint8_t ep_dir;
	uint32_t xfer_len;
	struct usbh_isoc_desc isoc_desc;

	if (p_ep == NULL) {
		return -EINVAL;
	}

	ep_type = usbh_ep_type_get(p_ep);
	ep_dir = usbh_ep_dir_get(p_ep);

	if ((ep_type != USBH_EP_TYPE_ISOC) || (ep_dir != USBH_EP_DIR_IN)) {
		return -EAGAIN;
	}

	isoc_desc.buf_ptr = p_buf;
	isoc_desc.buf_len = buf_len;
	isoc_desc.start_frm = start_frm;
	isoc_desc.nbr_frm = nbr_frm;
	isoc_desc.frm_len = p_frm_len;
	isoc_desc.frm_err = p_frm_err;

	xfer_len = usbh_sync_transfer(p_ep, p_buf, buf_len, &isoc_desc,
				      USBH_TOKEN_IN, timeout_ms, p_err);

	return xfer_len;
}

/*
 * Issue asynchronous isochronous request to receive data from device.
 */
int usbh_isoc_rx_async(struct usbh_ep *p_ep, uint8_t *p_buf, uint32_t buf_len,
		       uint32_t start_frm, uint32_t nbr_frm,
		       uint16_t *p_frm_len, int *p_frm_err,
		       usbh_isoc_cmpl_fnct fnct, void *p_fnct_arg)
{
	int err;
	uint8_t ep_type;
	uint8_t ep_dir;
	struct usbh_isoc_desc *p_isoc_desc;

	if (p_ep == NULL) {
		return -EINVAL;
	}

	ep_type = usbh_ep_type_get(p_ep);
	ep_dir = usbh_ep_dir_get(p_ep);

	if ((ep_type != USBH_EP_TYPE_ISOC) || (ep_dir != USBH_EP_DIR_IN)) {
		return -EAGAIN;
	}

	if (p_ep->dev_ptr->hc_ptr->host_ptr->isoc_cnt < 0) {
		return -ENOMEM;
	}
	p_isoc_desc =
		&p_ep->dev_ptr->hc_ptr->host_ptr
		->isoc_desc[p_ep->dev_ptr->hc_ptr->host_ptr->isoc_cnt--];

	p_isoc_desc->buf_ptr = p_buf;
	p_isoc_desc->buf_len = buf_len;
	p_isoc_desc->start_frm = start_frm;
	p_isoc_desc->nbr_frm = nbr_frm;
	p_isoc_desc->frm_len = p_frm_len;
	p_isoc_desc->frm_err = p_frm_err;

	err = usbh_async_transfer(p_ep, p_buf, buf_len, p_isoc_desc,
				  USBH_TOKEN_IN, (void *)fnct, p_fnct_arg);
	if (err != 0) {
		p_ep->dev_ptr->hc_ptr->host_ptr->isoc_cnt++;
	}

	return err;
}

/*
 * Get logical number of given endpoint.
 */
uint8_t usbh_ep_log_nbr_get(struct usbh_ep *p_ep)
{
	return ((uint8_t)p_ep->desc.b_endpoint_address & 0x7F);
}

/*
 * Get the direction of given endpoint.
 */
uint8_t usbh_ep_dir_get(struct usbh_ep *p_ep)
{
	uint8_t ep_type;

	ep_type = usbh_ep_type_get(p_ep);
	if (ep_type == USBH_EP_TYPE_CTRL) {
		return USBH_EP_DIR_NONE;
	}

	if (((uint8_t)p_ep->desc.b_endpoint_address & 0x80) != 0) {
		return USBH_EP_DIR_IN;
	} else {
		return USBH_EP_DIR_OUT;
	}
}

/*
 * Get the maximum packet size of the given endpoint
 */
uint16_t usbh_ep_max_pkt_size_get(struct usbh_ep *p_ep)
{
	return ((uint16_t)p_ep->desc.w_max_packet_size & 0x07FF);
}

/*
 * Get type of the given endpoint
 */
uint8_t usbh_ep_type_get(struct usbh_ep *p_ep)
{
	return ((uint8_t)p_ep->desc.bm_attributes & 0x03);
}

/*
 * Get endpoint specified by given index / alternate setting / interface.
 */
int usbh_ep_get(struct usbh_if *p_if, uint8_t alt_ix, uint8_t ep_ix,
		struct usbh_ep *p_ep)
{
	struct usbh_desc_hdr *p_desc;
	uint32_t if_off;
	uint8_t ix;

	if ((p_if == NULL) || (p_ep == NULL)) {
		return -EINVAL;
	}

	ix = 0;
	if_off = 0;
	p_desc = (struct usbh_desc_hdr *)p_if->if_data_ptr;

	while (if_off < p_if->if_data_len) {
		p_desc = usbh_next_desc_get((void *)p_desc, &if_off);

		if (p_desc->b_desc_type == USBH_DESC_TYPE_IF) {
			/* Chk if IF desc.*/
			if (alt_ix == ((uint8_t *)p_desc)[3]) {
				/* Compare alternate setting ix.*/
				break;
			}
		}
	}

	while (if_off < p_if->if_data_len) {
		p_desc = usbh_next_desc_get((void *)p_desc, &if_off);

		if (p_desc->b_desc_type == USBH_DESC_TYPE_EP) {
			/* Chk if EP desc.*/
			if (ix == ep_ix) {
				/* Compare EP ix.*/
				usbh_parse_ep_desc(&p_ep->desc, (void *)p_desc);
				return 0;
			}
			ix++;
		}
	}

	return -ENOENT;
}

/*
 * Set the STALL condition on endpoint.
 */
int usbh_ep_stall_set(struct usbh_ep *p_ep)
{
	int err;
	struct usbh_dev *p_dev;

	p_dev = p_ep->dev_ptr;

	(void)usbh_ctrl_tx(p_dev, USBH_REQ_SET_FEATURE,
			   USBH_REQ_DIR_HOST_TO_DEV | USBH_REQ_TYPE_STD |
			   USBH_REQ_RECIPIENT_EP,
			   USBH_FEATURE_SEL_EP_HALT,
			   p_ep->desc.b_endpoint_address, NULL, 0,
			   USBH_CFG_STD_REQ_TIMEOUT, &err);
	if (err != 0) {
		usbh_ep_reset(p_dev, NULL);
	}

	return err;
}

/*
 * Clear the STALL condition on endpoint.
 */
int usbh_ep_stall_clr(struct usbh_ep *p_ep)
{
	int err;
	struct usbh_dev *p_dev;

	p_dev = p_ep->dev_ptr;
	(void)usbh_ctrl_tx(p_dev, USBH_REQ_CLR_FEATURE,
			   USBH_REQ_DIR_HOST_TO_DEV | USBH_REQ_TYPE_STD |
			   USBH_REQ_RECIPIENT_EP,
			   USBH_FEATURE_SEL_EP_HALT,
			   p_ep->desc.b_endpoint_address, NULL, 0,
			   USBH_CFG_STD_REQ_TIMEOUT, &err);
	if (err != 0) {
		usbh_ep_reset(p_dev, NULL);
	}

	return err;
}

/*
 * This function is used to reset the opened endpoint
 */
int usbh_ep_reset(struct usbh_dev *p_dev, struct usbh_ep *p_ep)
{
	struct usbh_ep *p_ep_t;
	int err;

	if (p_ep == NULL) {
		p_ep_t = &p_dev->dflt_ep;
	} else {
		p_ep_t = p_ep;
	}
	/* Do nothing if virtual RH.*/
	if ((p_dev->is_root_hub == true) &&
	    (p_dev->hc_ptr->is_vir_rh == true)) {
		return 0;
	}

	k_mutex_lock(&p_dev->hc_ptr->hcd_mutex, K_NO_WAIT);
	p_dev->hc_ptr->hc_drv.api_ptr->ep_abort(&p_dev->hc_ptr->hc_drv, p_ep_t,
						&err);
	k_mutex_unlock(&p_dev->hc_ptr->hcd_mutex);
	if (err != 0) {
		return err;
	}

	k_mutex_lock(&p_dev->hc_ptr->hcd_mutex, K_NO_WAIT);
	p_dev->hc_ptr->hc_drv.api_ptr->ep_close(&p_dev->hc_ptr->hc_drv, p_ep_t,
						&err);
	k_mutex_unlock(&p_dev->hc_ptr->hcd_mutex);
	if (err != 0) {
		return err;
	}

	k_mutex_lock(&p_dev->hc_ptr->hcd_mutex, K_NO_WAIT);
	p_dev->hc_ptr->hc_drv.api_ptr->ep_open(&p_dev->hc_ptr->hc_drv, p_ep_t,
					       &err);
	k_mutex_unlock(&p_dev->hc_ptr->hcd_mutex);
	if (err != 0) {
		return err;
	}

	return 0;
}

/*
 * Closes given endpoint and makes it unavailable for I/O transfers.
 */
int usbh_ep_close(struct usbh_ep *p_ep)
{
	LOG_DBG("close endpoint");
	int err;
	struct usbh_dev *p_dev;
	struct usbh_urb *p_async_urb;

	if (p_ep == NULL) {
		return -EINVAL;
	}

	p_ep->is_open = false;
	p_dev = p_ep->dev_ptr;
	err = 0;

	if (!((p_dev->is_root_hub == true) &&
	      (p_dev->hc_ptr->is_vir_rh == true))) {
		/* Abort any pending urb.*/
		usbh_urb_abort(&p_ep->urb);
	}
	/* Close EP on HC.*/
	if (!((p_dev->is_root_hub == true) &&
	      (p_dev->hc_ptr->is_vir_rh == true))) {
		LOG_DBG("close address %d", ((p_ep->dev_addr << 8) |
					     p_ep->desc.b_endpoint_address));
		k_mutex_lock(&p_ep->dev_ptr->hc_ptr->hcd_mutex, K_NO_WAIT);
		p_ep->dev_ptr->hc_ptr->hc_drv.api_ptr->ep_close(
			&p_ep->dev_ptr->hc_ptr->hc_drv, p_ep, &err);
		k_mutex_unlock(&p_ep->dev_ptr->hc_ptr->hcd_mutex);
	}

	if (p_ep->xfer_nbr_in_prog > 1) {
		p_async_urb = p_ep->urb.async_urb_nxt_ptr;
		while (p_async_urb != 0) {
			/* Free extra urb.*/
			k_free(p_async_urb);

			p_async_urb = p_async_urb->async_urb_nxt_ptr;
		}

		p_ep->xfer_nbr_in_prog = 0;
	}

	return err;
}

/*
 * Handle a USB request block (urb) that has been completed by host controller.
 */
void usbh_urb_done(struct usbh_urb *p_urb)
{
	int key;

	if (p_urb->state == USBH_URB_STATE_SCHEDULED) {
		/* urb must be in scheduled state.*/
		/* Set urb state to done.*/
		p_urb->state = USBH_URB_STATE_QUEUED;

		if (p_urb->fnct_ptr != NULL) {
			/* Check if req is async.*/
			key = irq_lock();
			p_urb->nxt_ptr = NULL;

			if (usbh_urb_head_ptr == NULL) {
				usbh_urb_head_ptr = p_urb;
				usbh_urb_tail_ptr = p_urb;
			} else {
				usbh_urb_tail_ptr->nxt_ptr = p_urb;
				usbh_urb_tail_ptr = p_urb;
			}
			irq_unlock(key);

			k_sem_give(&usbh_urb_sem);
		} else {
			/* Post notification to waiting task.*/
			k_sem_give(&p_urb->sem);
		}
	}
}

/*
 * Handle a urb after transfer has been completed or aborted.
 */
int usbh_urb_complete(struct usbh_urb *p_urb)
{
	struct usbh_dev *p_dev;
	int err;
	struct usbh_urb *p_async_urb_to_remove;
	struct usbh_urb *p_prev_async_urb;
	struct usbh_urb urb_temp;
	struct usbh_ep *p_ep;
	int key;

	p_ep = p_urb->ep_ptr;
	p_dev = p_ep->dev_ptr;

	if (p_urb->state == USBH_URB_STATE_QUEUED) {
		k_mutex_lock(&p_dev->hc_ptr->hcd_mutex, K_NO_WAIT);
		p_dev->hc_ptr->hc_drv.api_ptr->urb_complete(
			&p_dev->hc_ptr->hc_drv, p_urb, &err);
		k_mutex_unlock(&p_dev->hc_ptr->hcd_mutex);
	} else if (p_urb->state == USBH_URB_STATE_ABORTED) {
		k_mutex_lock(&p_dev->hc_ptr->hcd_mutex, K_NO_WAIT);
		p_dev->hc_ptr->hc_drv.api_ptr->urb_abort(&p_dev->hc_ptr->hc_drv,
							 p_urb, &err);
		k_mutex_unlock(&p_dev->hc_ptr->hcd_mutex);

		p_urb->err = -EAGAIN;
		p_urb->xfer_len = 0;
	} else {
		/* Empty Else statement*/
	}
	/* Copy urb locally before freeing it.*/
	memcpy((void *)&urb_temp, (void *)p_urb, sizeof(struct usbh_urb));

	/*  FREE urb BEFORE NOTIFYING CLASS  */
	/* Is the urb an extra urb for async function?*/
	if ((p_urb != &p_ep->urb) && (p_urb->fnct_ptr != 0)) {
		p_async_urb_to_remove = &p_ep->urb;
		p_prev_async_urb = &p_ep->urb;

		while (p_async_urb_to_remove->async_urb_nxt_ptr != 0) {
			/* Srch extra urb to remove.*/
			/* Extra urb found*/
			if (p_async_urb_to_remove->async_urb_nxt_ptr == p_urb) {
				/* Remove from Q.*/
				p_prev_async_urb->async_urb_nxt_ptr =
					p_urb->async_urb_nxt_ptr;
				break;
			}
			p_prev_async_urb = p_async_urb_to_remove;
			p_async_urb_to_remove =
				p_async_urb_to_remove->async_urb_nxt_ptr;
		}
		/* Free extra urb.*/
		k_free(p_urb);
	}

	key = irq_lock();
	if (p_ep->xfer_nbr_in_prog > 0) {
		p_ep->xfer_nbr_in_prog--;
	}
	irq_unlock(key);

	if ((urb_temp.state == USBH_URB_STATE_QUEUED) ||
	    (urb_temp.state == USBH_URB_STATE_ABORTED)) {
		usb_urb_notify(&urb_temp);
	}

	return 0;
}

/*
 * Read specified string descriptor, remove header and extract string data.
 */
uint32_t usbh_str_get(struct usbh_dev *p_dev, uint8_t desc_ix, uint16_t lang_id,
		      uint8_t *p_buf, uint32_t buf_len, int *p_err)
{
	uint32_t ix;
	uint32_t str_len;
	uint8_t *p_str;
	struct usbh_desc_hdr *p_hdr;

	if (desc_ix == 0) {
		/* Invalid desc ix.*/
		*p_err = -EINVAL;
		return 0;
	}

	lang_id = p_dev->lang_id;

	if (lang_id == 0) {
		/* If lang ID is zero, get dflt used by the dev.*/
		str_len = usbh_str_desc_get(p_dev, 0, 0, p_buf, buf_len, p_err);
		if (str_len < 4u) {
			*p_err = -EINVAL;
			return 0;
		}
		/* Rd language ID into CPU endianness.*/
		lang_id = sys_get_le16((uint8_t *)&p_buf[2]);

		if (lang_id == 0) {
			*p_err = -EINVAL;
			return 0;
		}
		p_dev->lang_id = lang_id;
	}

	p_str = p_buf;
	p_hdr = (struct usbh_desc_hdr *)p_buf;
	/* Rd str desc with lang ID.*/
	str_len = usbh_str_desc_get(p_dev, desc_ix, lang_id, p_hdr, buf_len,
				    p_err);

	if (str_len > USBH_LEN_DESC_HDR) {
		/* Remove 2-byte header.*/
		str_len = (p_hdr->b_length - 2);

		if (str_len > (buf_len - 2)) {
			str_len = (buf_len - 2);
		}

		for (ix = 0; ix < str_len; ix++) {
			/* Str starts from byte 3 in desc.*/
			p_str[ix] = p_str[2 + ix];
		}

		p_str[ix] = 0;
		p_str[++ix] = 0;
		/* Len of ANSI str.*/
		str_len = str_len / 2;

		return str_len;
	}
	*p_err = -EINVAL;
	return 0;
}

/*
 * Open an endpoint.
 */
static int usbh_ep_open(struct usbh_dev *p_dev, struct usbh_if *p_if,
			uint8_t ep_type, uint8_t ep_dir, struct usbh_ep *p_ep)
{
	uint8_t ep_desc_dir;
	uint8_t ep_desc_type;
	uint8_t ep_ix;
	uint8_t nbr_eps;
	bool ep_found;
	int err;

	if (p_ep->is_open == true) {
		return 0;
	}

	usbh_urb_clr(&p_ep->urb);

	ep_found = false;
	ep_desc_type = 0;
	nbr_eps = usbh_if_ep_nbr_get(p_if, p_if->alt_ix_sel);

	if (nbr_eps > USBH_CFG_MAX_NBR_EPS) {
		err = EBUSY;
		return err;
	}

	for (ep_ix = 0; ep_ix < nbr_eps; ep_ix++) {
		usbh_ep_get(p_if, p_if->alt_ix_sel, ep_ix, p_ep);
		/* EP type from desc.*/
		ep_desc_type = p_ep->desc.bm_attributes & 0x03u;
		/* EP dir from desc.*/
		ep_desc_dir = p_ep->desc.b_endpoint_address & 0x80u;

		if (ep_desc_type == ep_type) {
			if ((ep_desc_type == USBH_EP_TYPE_CTRL) ||
			    (ep_desc_dir == ep_dir)) {
				ep_found = true;
				break;
			}
		}
	}
	if (ep_found == false) {
		/* Class specified EP not found.*/
		return -ENOENT;
	}

	p_ep->interval = 0;
	if (ep_desc_type == USBH_EP_TYPE_INTR) {
		/*  DETERMINE POLLING INTERVAL  */

		if (p_ep->desc.b_interval > 0) {
			if ((p_dev->dev_spd == USBH_LOW_SPEED) ||
			    (p_dev->dev_spd == USBH_FULL_SPEED)) {
				if (p_dev->hub_hs_ptr != NULL) {
					/* 1 (1ms)frame = 8 (125us)microframe.*/
					p_ep->interval =
						8 * p_ep->desc.b_interval;
				} else {
					p_ep->interval = p_ep->desc.b_interval;
				}
			} else {
				/* dev_spd == USBH_DEV_SPD_HIGH*/
				/* For HS, interval is 2 ^ (b_interval - 1).*/
				p_ep->interval = 1
						 << (p_ep->desc.b_interval - 1);
			}
		}
	} else if (ep_desc_type == USBH_EP_TYPE_ISOC) {
		/* Isoc interval is 2 ^ (b_interval - 1).*/
		p_ep->interval = 1 << (p_ep->desc.b_interval - 1);
	} else {
		/* Empty Else statement*/
	}

	p_ep->dev_addr = p_dev->dev_addr;
	p_ep->dev_spd = p_dev->dev_spd;
	p_ep->dev_ptr = p_dev;

	if (!((p_dev->is_root_hub == true) &&
	      (p_dev->hc_ptr->is_vir_rh == true))) {
		k_mutex_lock(&p_dev->hc_ptr->hcd_mutex, K_NO_WAIT);
		p_dev->hc_ptr->hc_drv.api_ptr->ep_open(&p_dev->hc_ptr->hc_drv,
						       p_ep, &err);
		k_mutex_unlock(&p_dev->hc_ptr->hcd_mutex);
		if (err != 0) {
			return err;
		}
	}
	/* sem for I/O wait.*/
	err = k_sem_init(&p_ep->urb.sem, 0, USBH_OS_SEM_REQUIRED);
	if (err != 0) {
		return err;
	}

	k_mutex_init(&p_ep->mutex);

	p_ep->is_open = true;
	p_ep->urb.ep_ptr = p_ep;

	return err;
}

/*
 * Perform synchronous transfer on endpoint.
 */
static uint32_t usbh_sync_transfer(struct usbh_ep *p_ep, void *p_buf,
				   uint32_t buf_len,
				   struct usbh_isoc_desc *p_isoc_desc,
				   uint8_t token, uint32_t timeout_ms,
				   int *p_err)
{
	uint32_t len;
	struct usbh_urb *p_urb;

	/* Argument checks for valid settings*/
	if (p_ep == NULL) {
		*p_err = -EINVAL;
		return 0;
	}

	if (p_ep->is_open == false) {
		*p_err = -EAGAIN;
		return 0;
	}

	k_mutex_lock(&p_ep->mutex, K_NO_WAIT);

	p_urb = &p_ep->urb;
	p_urb->ep_ptr = p_ep;
	p_urb->isoc_desc_ptr = p_isoc_desc;
	p_urb->userbuf_ptr = p_buf;
	p_urb->uberbuf_len = buf_len;
	p_urb->dma_buf_len = 0;
	p_urb->dma_buf_ptr = NULL;
	p_urb->xfer_len = 0;
	p_urb->fnct_ptr = 0;
	p_urb->fnct_arg_ptr = 0;
	p_urb->state = USBH_URB_STATE_NONE;
	p_urb->arg_ptr = NULL;
	p_urb->token = token;

	*p_err = usbh_urb_submit(p_urb);

	if (*p_err == 0) {
		/* Transfer urb to HC.*/
		/* Wait on urb completion notification.*/
		*p_err = k_sem_take(&p_urb->sem, K_MSEC(timeout_ms));
	}

	if (*p_err == 0) {
		usbh_urb_complete(p_urb);
		*p_err = p_urb->err;
	} else {
		usbh_urb_abort(p_urb);
	}

	len = p_urb->xfer_len;
	p_urb->state = USBH_URB_STATE_NONE;
	k_mutex_unlock(&p_ep->mutex);

	return len;
}

/*
 * Perform asynchronous transfer on endpoint.
 */
static int usbh_async_transfer(struct usbh_ep *p_ep, void *p_buf,
			       uint32_t buf_len,
			       struct usbh_isoc_desc *p_isoc_desc,
			       uint8_t token, void *p_fnct, void *p_fnct_arg)
{
	int err;
	struct usbh_urb *p_urb;
	struct usbh_urb *p_async_urb;
	struct k_mem_pool *p_async_urb_pool;

	if (p_ep->is_open == false) {
		return -EAGAIN;
	}
	/* Chk if no xfer is pending or in progress on EP.*/
	if ((p_ep->urb.state != USBH_URB_STATE_SCHEDULED) &&
	    (p_ep->xfer_nbr_in_prog == 0)) {
		/* Use urb struct associated to EP.*/
		p_urb = &p_ep->urb;
	} else if (p_ep->xfer_nbr_in_prog >= 1) {
		/* Get a new urb struct from the urb async pool.*/
		p_async_urb_pool =
			&p_ep->dev_ptr->hc_ptr->host_ptr->async_urb_pool;
		p_urb = k_mem_pool_malloc(p_async_urb_pool,
					  sizeof(struct usbh_urb));
		if (p_urb == NULL) {
			return -ENOMEM;
		}

		usbh_urb_clr(p_urb);
		/* Get head of extra async urb Q.*/
		p_async_urb = &p_ep->urb;

		while (p_async_urb->async_urb_nxt_ptr != 0) {
			/* Srch tail of extra async urb Q.*/
			p_async_urb = p_async_urb->async_urb_nxt_ptr;
		}
		/* Insert new urb at end of extra async urb Q.*/
		p_async_urb->async_urb_nxt_ptr = p_urb;
	} else {
		return -EAGAIN;
	}
	p_ep->xfer_nbr_in_prog++;

	p_urb->ep_ptr = p_ep;
	/*  PREPARE urb  */
	p_urb->isoc_desc_ptr = p_isoc_desc;
	p_urb->userbuf_ptr = p_buf;
	p_urb->uberbuf_len = buf_len;
	p_urb->xfer_len = 0;
	p_urb->fnct_ptr = (void *)p_fnct;
	p_urb->fnct_arg_ptr = p_fnct_arg;
	p_urb->state = USBH_URB_STATE_NONE;
	p_urb->arg_ptr = NULL;
	p_urb->token = token;

	err = usbh_urb_submit(p_urb);

	return err;
}

/*
 * Perform synchronous control transfer on endpoint.
 */
static uint16_t usbh_sync_ctrl_transfer(struct usbh_ep *p_ep, uint8_t b_req,
					uint8_t bm_req_type, uint16_t w_val,
					uint16_t w_ix, void *p_arg,
					uint16_t w_len, uint32_t timeout_ms,
					int *p_err)
{
	struct usbh_setup_req setup;
	uint8_t setup_buf[8];
	bool is_in;
	uint32_t len;
	uint16_t rtn_len;
	uint8_t *p_data_08;

	setup.bm_request_type = bm_req_type;
	/*  SETUP STAGE  */
	setup.b_request = b_req;
	setup.w_value = w_val;
	setup.w_index = w_ix;
	setup.w_length = w_len;

	usbh_fmt_setup_req(&setup, setup_buf);
	is_in = (bm_req_type & USBH_REQ_DIR_MASK) != 0 ? true : false;

	len = usbh_sync_transfer(p_ep, (void *)&setup_buf[0],
				 USBH_LEN_SETUP_PKT, NULL, USBH_TOKEN_SETUP,
				 timeout_ms, p_err);
	if (*p_err != 0) {
		return 0;
	}

	if (len != USBH_LEN_SETUP_PKT) {
		*p_err = -EAGAIN;
		return 0;
	}

	if (w_len > 0) {
		/*  DATA STAGE  */
		p_data_08 = (uint8_t *)p_arg;

		rtn_len = usbh_sync_transfer(
			p_ep, (void *)p_data_08, w_len, NULL,
			(is_in ? USBH_TOKEN_IN : USBH_TOKEN_OUT), timeout_ms,
			p_err);
		if (*p_err != 0) {
			return 0;
		}
	} else {
		rtn_len = 0;
	}
	/*  STATUS STAGE  */
	(void)usbh_sync_transfer(p_ep, NULL, 0, NULL,
				 ((w_len && is_in) ? USBH_TOKEN_OUT :
				  USBH_TOKEN_IN),
				 timeout_ms, p_err);
	if (*p_err != 0) {
		return 0;
	}

	return rtn_len;
}

/*
 * Abort pending urb.
 */
static void usbh_urb_abort(struct usbh_urb *p_urb)
{
	bool cmpl;
	int key;

	cmpl = false;

	key = irq_lock();

	if (p_urb->state == USBH_URB_STATE_SCHEDULED) {
		/* Abort scheduled urb.*/
		p_urb->state = USBH_URB_STATE_ABORTED;
		/* Mark urb as completion pending.*/
		cmpl = true;
	} else if (p_urb->state == USBH_URB_STATE_QUEUED) {
		/* Is urb queued in async Q?*/
		/* urb is in async lst.*/
		p_urb->state = USBH_URB_STATE_ABORTED;
	} else {
		/* Empty Else statement*/
	}
	irq_unlock(key);

	if (cmpl == true) {
		usbh_urb_complete(p_urb);
	}
}

/*
 * Notify application about state of given urb.
 */
static void usb_urb_notify(struct usbh_urb *p_urb)
{
	uint16_t nbr_frm;
	uint16_t *p_frm_len;
	uint32_t buf_len;
	uint32_t xfer_len;
	uint32_t start_frm;
	void *p_buf;
	void *p_arg;
	struct usbh_ep *p_ep;
	struct usbh_isoc_desc *p_isoc_desc;
	usbh_xfer_cmpl_fnct p_xfer_fnct;
	usbh_isoc_cmpl_fnct p_isoc_fnct;
	int *p_frm_err;
	int err;
	int key;

	p_ep = p_urb->ep_ptr;
	p_isoc_desc = p_urb->isoc_desc_ptr;

	key = irq_lock();
	if ((p_urb->state == USBH_URB_STATE_ABORTED) &&
	    (p_urb->fnct_ptr == NULL)) {
		p_urb->state = USBH_URB_STATE_NONE;
		k_sem_reset(&p_urb->sem);
	}

	if (p_urb->fnct_ptr != NULL) {
		/*  Save urb info.*/

		p_buf = p_urb->userbuf_ptr;
		buf_len = p_urb->uberbuf_len;
		xfer_len = p_urb->xfer_len;
		p_arg = p_urb->fnct_arg_ptr;
		err = p_urb->err;
		p_urb->state = USBH_URB_STATE_NONE;

		if (p_isoc_desc == NULL) {
			p_xfer_fnct = (usbh_xfer_cmpl_fnct)p_urb->fnct_ptr;
			irq_unlock(key);

			p_xfer_fnct(p_ep, p_buf, buf_len, xfer_len, p_arg, err);
		} else {
			p_isoc_fnct = (usbh_isoc_cmpl_fnct)p_urb->fnct_ptr;
			start_frm = p_isoc_desc->start_frm;
			nbr_frm = p_isoc_desc->nbr_frm;
			p_frm_len = p_isoc_desc->frm_len;
			p_frm_err = p_isoc_desc->frm_err;
			irq_unlock(key);

			p_ep->dev_ptr->hc_ptr->host_ptr->isoc_cnt++;

			p_isoc_fnct(p_ep, p_buf, buf_len, xfer_len, start_frm,
				    nbr_frm, p_frm_len, p_frm_err, p_arg, err);
		}
	} else {
		irq_unlock(key);
	}
}

/*
 * Submit given urb to host controller.
 */
static int usbh_urb_submit(struct usbh_urb *p_urb)
{
	int err;
	bool ep_is_halt;
	struct usbh_dev *p_dev;

	p_dev = p_urb->ep_ptr->dev_ptr;
	k_mutex_lock(&p_dev->hc_ptr->hcd_mutex, K_NO_WAIT);
	ep_is_halt = p_dev->hc_ptr->hc_drv.api_ptr->ep_halt(
		&p_dev->hc_ptr->hc_drv, p_urb->ep_ptr, &err);
	k_mutex_unlock(&p_dev->hc_ptr->hcd_mutex);
	if ((ep_is_halt == true) && (err == 0)) {
		return -EAGAIN;
	}
	/* Set urb state to scheduled.*/
	p_urb->state = USBH_URB_STATE_SCHEDULED;
	p_urb->err = 0;

	k_mutex_lock(&p_dev->hc_ptr->hcd_mutex, K_NO_WAIT);
	p_dev->hc_ptr->hc_drv.api_ptr->urb_submit(&p_dev->hc_ptr->hc_drv, p_urb,
						  &err);
	k_mutex_unlock(&p_dev->hc_ptr->hcd_mutex);

	return err;
}

/*
 * Clear urb structure
 */
static void usbh_urb_clr(struct usbh_urb *p_urb)
{
	p_urb->err = 0;
	p_urb->state = USBH_URB_STATE_NONE;
	p_urb->async_urb_nxt_ptr = NULL;
}

/*
 * Open default control endpoint of given USB device.
 */
static int usbh_dflt_ep_open(struct usbh_dev *p_dev)
{
	uint16_t ep_max_pkt_size;
	struct usbh_ep *p_ep;
	int err;

	p_ep = &p_dev->dflt_ep;

	if (p_ep->is_open == true) {
		return 0;
	}

	p_ep->dev_addr = 0;
	p_ep->dev_spd = p_dev->dev_spd;
	p_ep->dev_ptr = p_dev;

	if (p_dev->dev_spd == USBH_LOW_SPEED) {
		ep_max_pkt_size = 8;
	} else {
		ep_max_pkt_size = 64u;
	}

	p_ep->desc.b_length = 7u;
	p_ep->desc.b_desc_type = USBH_DESC_TYPE_EP;
	p_ep->desc.b_endpoint_address = 0;
	p_ep->desc.bm_attributes = USBH_EP_TYPE_CTRL;
	p_ep->desc.w_max_packet_size = ep_max_pkt_size;
	p_ep->desc.b_interval = 0;
	/* Chk if RH fncts are supported before calling HCD.*/
	if (!((p_dev->is_root_hub == true) &&
	      (p_dev->hc_ptr->is_vir_rh == true))) {
		k_mutex_lock(&p_dev->hc_ptr->hcd_mutex, K_NO_WAIT);
		p_dev->hc_ptr->hc_drv.api_ptr->ep_open(&p_dev->hc_ptr->hc_drv,
						       p_ep, &err);
		k_mutex_unlock(&p_dev->hc_ptr->hcd_mutex);
		if (err != 0) {
			return err;
		}
	}
	/* Create OS resources needed for EP.*/
	err = k_sem_init(&p_ep->urb.sem, 0, USBH_OS_SEM_REQUIRED);
	if (err != 0) {
		return err;
	}

	k_mutex_init(&p_ep->mutex);

	p_ep->urb.ep_ptr = p_ep;
	p_ep->is_open = true;

	return err;
}

/*
 * Read device descriptor from device.
 */
static int usbh_dev_desc_rd(struct usbh_dev *p_dev)
{
	int err;
	uint8_t retry;
	struct usbh_dev_desc dev_desc;

	retry = 3u;
	while (retry > 0) {
		retry--;
		/*  READ FIRST 8 BYTES OF DEV DESC  */
		usbh_ctrl_tx(p_dev, USBH_REQ_GET_DESC,
			     (USBH_REQ_DIR_DEV_TO_HOST |
			      USBH_REQ_RECIPIENT_DEV),
			     (USBH_DESC_TYPE_DEV << 8) | 0, 0, p_dev->dev_desc,
			     8, USBH_CFG_STD_REQ_TIMEOUT, &err);
		if (err != 0) {
			usbh_ep_reset(p_dev, NULL);
			k_sleep(K_MSEC(100u));
		} else {
			break;
		}
	}
	if (err != 0) {
		return err;
	}

	if (!((p_dev->is_root_hub == true) &&
	      (p_dev->hc_ptr->is_vir_rh == true))) {
		/* Retrieve EP 0 max pkt size.*/
		p_dev->dflt_ep.desc.w_max_packet_size =
			(uint8_t)p_dev->dev_desc[7u];
		if (p_dev->dflt_ep.desc.w_max_packet_size > 64u) {
			return -EINVAL;
		}

		k_mutex_lock(&p_dev->hc_ptr->hcd_mutex, K_NO_WAIT);
		p_dev->hc_ptr->hc_drv.api_ptr->ep_close(&p_dev->hc_ptr->hc_drv,
							&p_dev->dflt_ep, &err);
		k_mutex_unlock(&p_dev->hc_ptr->hcd_mutex);

		k_mutex_lock(&p_dev->hc_ptr->hcd_mutex, K_NO_WAIT);
		p_dev->hc_ptr->hc_drv.api_ptr->ep_open(&p_dev->hc_ptr->hc_drv,
						       &p_dev->dflt_ep, &err);
		k_mutex_unlock(&p_dev->hc_ptr->hcd_mutex);
		if (err != 0) {
			return err;
		}
	}

	retry = 3u;
	while (retry > 0) {
		retry--;
		/*  RD FULL DEV DESC.  */
		usbh_ctrl_tx(p_dev, USBH_REQ_GET_DESC,
			     (USBH_REQ_DIR_DEV_TO_HOST |
			      USBH_REQ_RECIPIENT_DEV),
			     (USBH_DESC_TYPE_DEV << 8) | 0, 0, p_dev->dev_desc,
			     USBH_LEN_DESC_DEV, USBH_CFG_STD_REQ_TIMEOUT, &err);
		if (err != 0) {
			usbh_ep_reset(p_dev, NULL);
			k_sleep(K_MSEC(100u));
		} else {
			break;
		}
	}
	if (err != 0) {
		return err;
	}

	/*  VALIDATE DEV DESC  */
	usbh_dev_desc_get(p_dev, &dev_desc);

	if ((dev_desc.b_length < USBH_LEN_DESC_DEV) ||
	    (dev_desc.b_desc_type != USBH_DESC_TYPE_DEV) ||
	    (dev_desc.b_nbr_configs == 0)) {
		return -EINVAL;
	}

	if ((dev_desc.b_device_class != USBH_CLASS_CODE_USE_IF_DESC) &&
	    (dev_desc.b_device_class != USBH_CLASS_CODE_AUDIO) &&
	    (dev_desc.b_device_class != USBH_CLASS_CODE_CDC_CTRL) &&
	    (dev_desc.b_device_class != USBH_CLASS_CODE_HID) &&
	    (dev_desc.b_device_class != USBH_CLASS_CODE_PHYSICAL) &&
	    (dev_desc.b_device_class != USBH_CLASS_CODE_IMAGE) &&
	    (dev_desc.b_device_class != USBH_CLASS_CODE_PRINTER) &&
	    (dev_desc.b_device_class != USBH_CLASS_CODE_MASS_STORAGE) &&
	    (dev_desc.b_device_class != USBH_CLASS_CODE_HUB) &&
	    (dev_desc.b_device_class != USBH_CLASS_CODE_CDC_DATA) &&
	    (dev_desc.b_device_class != USBH_CLASS_CODE_SMART_CARD) &&
	    (dev_desc.b_device_class != USBH_CLASS_CODE_CONTENT_SECURITY) &&
	    (dev_desc.b_device_class != USBH_CLASS_CODE_VIDEO) &&
	    (dev_desc.b_device_class != USBH_CLASS_CODE_PERSONAL_HEALTHCARE) &&
	    (dev_desc.b_device_class != USBH_CLASS_CODE_DIAGNOSTIC_DEV) &&
	    (dev_desc.b_device_class != USBH_CLASS_CODE_WIRELESS_CTRLR) &&
	    (dev_desc.b_device_class != USBH_CLASS_CODE_MISCELLANEOUS) &&
	    (dev_desc.b_device_class != USBH_CLASS_CODE_APP_SPECIFIC) &&
	    (dev_desc.b_device_class != USBH_CLASS_CODE_VENDOR_SPECIFIC)) {
		return -EINVAL;
	}

	return 0;
}

/*
 * Read configuration descriptor for a given configuration number.
 */
static int usbh_cfg_rd(struct usbh_dev *p_dev, uint8_t cfg_ix)
{
	uint16_t w_tot_len;
	uint16_t b_read;
	int err;
	struct usbh_cfg *p_cfg;
	uint8_t retry;

	p_cfg = usbh_cfg_get(p_dev, cfg_ix);
	if (p_cfg == NULL) {
		return -ENOMEM;
	}

	retry = 3u;
	while (retry > 0) {
		retry--;
		b_read = usbh_ctrl_tx(
			p_dev, USBH_REQ_GET_DESC,
			(USBH_REQ_DIR_DEV_TO_HOST | USBH_REQ_RECIPIENT_DEV),
			(USBH_DESC_TYPE_CFG << 8) | cfg_ix, 0, p_cfg->cfg_data,
			USBH_LEN_DESC_CFG, USBH_CFG_STD_REQ_TIMEOUT, &err);
		if (err != 0) {
			usbh_ep_reset(p_dev, NULL);
			k_sleep(K_MSEC(100u));
		} else {
			break;
		}
	}
	if (err != 0) {
		LOG_ERR("---");
		return err;
	}

	if (b_read < USBH_LEN_DESC_CFG) {
		LOG_ERR("bread < %d", USBH_LEN_DESC_CFG);
		return -EINVAL;
	}

	if (p_cfg->cfg_data[1] != USBH_DESC_TYPE_CFG) {
		LOG_ERR("desc invalid");
		return -EINVAL;
	}

	w_tot_len = sys_get_le16((uint8_t *)&p_cfg->cfg_data[2]);

	if (w_tot_len > USBH_CFG_MAX_CFG_DATA_LEN) {
		/* Chk total len of config desc.*/
		LOG_ERR("w_tot_len > %d", USBH_CFG_MAX_CFG_DATA_LEN);
		return -ENOMEM;
	}

	retry = 3u;
	while (retry > 0) {
		retry--;
		b_read = usbh_ctrl_tx(
			p_dev, USBH_REQ_GET_DESC,
			(USBH_REQ_DIR_DEV_TO_HOST | USBH_REQ_RECIPIENT_DEV),
			(USBH_DESC_TYPE_CFG << 8) | cfg_ix, 0, p_cfg->cfg_data,
			w_tot_len, USBH_CFG_STD_REQ_TIMEOUT, &err);
		if (err != 0) {
			LOG_ERR("err get full descriptor");
			usbh_ep_reset(p_dev, NULL);
			k_sleep(K_MSEC(100u));
		} else {
			break;
		}
	}
	if (err != 0) {
		LOG_ERR("...");
		return err;
	}

	if (b_read < w_tot_len) {
		LOG_ERR("bread < w_tot_len");
		return -EINVAL;
	}

	if (p_cfg->cfg_data[1] != USBH_DESC_TYPE_CFG) {
		/* Validate config desc.*/
		LOG_ERR("desc type ivalid");
		return -EINVAL;
	}

	p_cfg->cfg_data_len = w_tot_len;
	err = usbh_cfg_parse(p_dev, p_cfg);
	return err;
}

/*
 * Parse given configuration descriptor.
 */
static int usbh_cfg_parse(struct usbh_dev *p_dev, struct usbh_cfg *p_cfg)
{
	int8_t if_ix;
	uint8_t nbr_ifs;
	uint32_t cfg_off;
	struct usbh_if *p_if;
	struct usbh_desc_hdr *p_desc;
	struct usbh_cfg_desc cfg_desc;
	struct usbh_if_desc if_desc;
	struct usbh_ep_desc ep_desc;

	cfg_off = 0;
	p_desc = (struct usbh_desc_hdr *)p_cfg->cfg_data;

	/*  VALIDATE CFG DESC  */
	usbh_parse_cfg_desc(&cfg_desc, p_desc);
	if ((cfg_desc.b_max_pwr > 250u) || (cfg_desc.b_nbr_interfaces == 0)) {
		return -EINVAL;
	}
	/* nbr of IFs present in config.*/
	nbr_ifs = usbh_cfg_if_nbr_get(p_cfg);
	if (nbr_ifs > USBH_CFG_MAX_NBR_IFS) {
		return -ENOMEM;
	}

	if_ix = 0;
	p_if = NULL;

	while (cfg_off < p_cfg->cfg_data_len) {
		p_desc = usbh_next_desc_get((void *)p_desc, &cfg_off);

		/*  VALIDATE INTERFACE DESCRIPTOR  */
		if (p_desc->b_desc_type == USBH_DESC_TYPE_IF) {
			usbh_parse_if_desc(&if_desc, p_desc);
			if ((if_desc.b_if_class != USBH_CLASS_CODE_AUDIO) &&
			    (if_desc.b_if_class != USBH_CLASS_CODE_CDC_CTRL) &&
			    (if_desc.b_if_class != USBH_CLASS_CODE_HID) &&
			    (if_desc.b_if_class != USBH_CLASS_CODE_PHYSICAL) &&
			    (if_desc.b_if_class != USBH_CLASS_CODE_IMAGE) &&
			    (if_desc.b_if_class != USBH_CLASS_CODE_PRINTER) &&
			    (if_desc.b_if_class !=
			     USBH_CLASS_CODE_MASS_STORAGE) &&
			    (if_desc.b_if_class != USBH_CLASS_CODE_HUB) &&
			    (if_desc.b_if_class != USBH_CLASS_CODE_CDC_DATA) &&
			    (if_desc.b_if_class !=
			     USBH_CLASS_CODE_SMART_CARD) &&
			    (if_desc.b_if_class !=
			     USBH_CLASS_CODE_CONTENT_SECURITY) &&
			    (if_desc.b_if_class != USBH_CLASS_CODE_VIDEO) &&
			    (if_desc.b_if_class !=
			     USBH_CLASS_CODE_PERSONAL_HEALTHCARE) &&
			    (if_desc.b_if_class !=
			     USBH_CLASS_CODE_DIAGNOSTIC_DEV) &&
			    (if_desc.b_if_class !=
			     USBH_CLASS_CODE_WIRELESS_CTRLR) &&
			    (if_desc.b_if_class !=
			     USBH_CLASS_CODE_MISCELLANEOUS) &&
			    (if_desc.b_if_class !=
			     USBH_CLASS_CODE_APP_SPECIFIC) &&
			    (if_desc.b_if_class !=
			     USBH_CLASS_CODE_VENDOR_SPECIFIC)) {
				return -EINVAL;
			}

			if ((if_desc.b_nbr_endpoints > 30u)) {
				return -EINVAL;
			}

			if (if_desc.b_alt_setting == 0) {
				p_if = &p_cfg->if_list[if_ix];
				p_if->dev_ptr = (struct usbh_dev *)p_dev;
				p_if->if_data_ptr = (uint8_t *)p_desc;
				p_if->if_data_len = 0;
				if_ix++;
			}
		}

		if (p_desc->b_desc_type == USBH_DESC_TYPE_EP) {
			usbh_parse_ep_desc(&ep_desc, p_desc);

			if ((ep_desc.b_endpoint_address == 0x00u) ||
			    (ep_desc.b_endpoint_address == 0x80u) ||
			    (ep_desc.w_max_packet_size == 0)) {
				return -EINVAL;
			}
		}

		if (p_if != NULL) {
			p_if->if_data_len += p_desc->b_length;
		}
	}

	if (if_ix != nbr_ifs) {
		/* IF count must match max nbr of IFs.*/
		return -EINVAL;
	}

	return 0;
}

/*
 * Assign address to given USB device.
 */
static int usbh_dev_addr_set(struct usbh_dev *p_dev)
{
	LOG_DBG("set device address");
	int err;
	uint8_t retry;

	retry = 3u;
	while (retry > 0) {
		retry--;

		usbh_ctrl_tx(p_dev, USBH_REQ_SET_ADDR,
			     (USBH_REQ_DIR_HOST_TO_DEV |
			      USBH_REQ_RECIPIENT_DEV),
			     p_dev->dev_addr, 0, NULL, 0,
			     USBH_CFG_STD_REQ_TIMEOUT, &err);
		if (err != 0) {
			usbh_ep_reset(p_dev, NULL);
			k_sleep(K_MSEC(100u));
		} else {
			break;
		}
	}
	if (err != 0) {
		return err;
	}

	if ((p_dev->is_root_hub == true) &&
	    (p_dev->hc_ptr->is_vir_rh == true)) {
		return 0;
	}

	k_mutex_lock(&p_dev->hc_ptr->hcd_mutex, K_NO_WAIT);
	p_dev->hc_ptr->hc_drv.api_ptr->ep_close(&p_dev->hc_ptr->hc_drv,
						&p_dev->dflt_ep, &err);
	k_mutex_unlock(&p_dev->hc_ptr->hcd_mutex);
	/* Update addr.*/
	p_dev->dflt_ep.dev_addr = p_dev->dev_addr;

	k_mutex_lock(&p_dev->hc_ptr->hcd_mutex, K_NO_WAIT);
	p_dev->hc_ptr->hc_drv.api_ptr->ep_open(&p_dev->hc_ptr->hc_drv,
					       &p_dev->dflt_ep, &err);
	k_mutex_unlock(&p_dev->hc_ptr->hcd_mutex);
	if (err != 0) {
		return err;
	}

	k_sleep(K_MSEC(2));

	return err;
}

/*
 * Read specified string descriptor from USB device.
 */
static uint32_t usbh_str_desc_get(struct usbh_dev *p_dev, uint8_t desc_ix,
				  uint16_t lang_id, void *p_buf,
				  uint32_t buf_len, int *p_err)
{
	uint32_t len;
	uint32_t req_len;
	uint8_t i;
	struct usbh_desc_hdr *p_hdr;

	if (desc_ix == USBH_STRING_DESC_LANGID) {
		/* Size of lang ID = 4.*/
		req_len = 0x04u;
	} else {
		req_len = USBH_LEN_DESC_HDR;
	}
	req_len = MIN(req_len, buf_len);

	for (i = 0; i < USBH_CFG_STD_REQ_RETRY; i++) {
		/* Retry up to 3 times.*/
		len = usbh_ctrl_rx(
			p_dev, USBH_REQ_GET_DESC,
			USBH_REQ_DIR_DEV_TO_HOST | USBH_REQ_RECIPIENT_DEV,
			(USBH_DESC_TYPE_STR << 8) | desc_ix, lang_id, p_buf,
			req_len, USBH_CFG_STD_REQ_TIMEOUT, p_err);
		if ((len == 0) || (*p_err == EBUSY)) {
			/* Rst EP to clr HC halt state.*/
			usbh_ep_reset(p_dev, NULL);
		} else {
			break;
		}
	}

	if (*p_err != 0) {
		return 0;
	}

	p_hdr = (struct usbh_desc_hdr *)p_buf;
	/* Chk desc hdr.*/
	if ((len == req_len) && (p_hdr->b_length != 0) &&
	    (p_hdr->b_desc_type == USBH_DESC_TYPE_STR)) {
		len = p_hdr->b_length;
		if (desc_ix == USBH_STRING_DESC_LANGID) {
			return len;
		}
	} else {
		*p_err = -EINVAL;
		return 0;
	}

	if (len > buf_len) {
		len = buf_len;
	}
	/* Get full str desc.*/
	for (i = 0; i < USBH_CFG_STD_REQ_RETRY; i++) {
		/* Retry up to 3 times.*/
		len = usbh_ctrl_rx(p_dev, USBH_REQ_GET_DESC,
				   USBH_REQ_DIR_DEV_TO_HOST |
				   USBH_REQ_RECIPIENT_DEV,
				   (USBH_DESC_TYPE_STR << 8) | desc_ix, lang_id,
				   p_buf, len, USBH_CFG_STD_REQ_TIMEOUT, p_err);

		if ((len == 0) || (*p_err == EBUSY)) {
			usbh_ep_reset(p_dev, NULL);
		} else {
			break;
		}
	}

	if (*p_err != 0) {
		return 0;
	}

	if (len == 0) {
		*p_err = -EINVAL;
		return 0;
	}

	return len;
}

/*
 * Print specified string index to default output terminal.
 */
static void usbh_str_desc_print(struct usbh_dev *p_dev, uint8_t *str_prefix,
				uint8_t desc_ix)
{
	int err;
	uint32_t str_len;
	uint8_t str[USBH_CFG_MAX_STR_LEN];
	uint16_t ch;
	uint32_t ix;
	uint32_t buf_len;

	str_len = usbh_str_get(p_dev, desc_ix, USBH_STRING_DESC_LANGID, &str[0],
			       USBH_CFG_MAX_STR_LEN, &err);
	/* Print prefix str.*/
	printk("%s", str_prefix);

	if (str_len > 0u) {
		/* Print unicode string rd from the dev.*/
		buf_len = str_len * 2u;
		for (ix = 0u; (buf_len - ix) >= 2u; ix += 2u) {
			ch = sys_get_le16(&str[ix]);
			if (ch == 0u) {
				break;
			}
			printk("%c", ch);
		}
	}
	printk("\r\n");
}

/*
 * Description : Get pointer to next descriptor in
 *  buffer that contains configuration data.
 */
static struct usbh_desc_hdr *usbh_next_desc_get(void *p_buf, uint32_t *p_offset)
{
	struct usbh_desc_hdr *p_next_hdr;
	struct usbh_desc_hdr *p_hdr;

	/* Current desc hdr.*/
	p_hdr = (struct usbh_desc_hdr *)p_buf;

	if (*p_offset == 0) {
		/* 1st desc in buf.*/
		p_next_hdr = p_hdr;
	} else {
		/* Next desc is at end of current desc.*/
		p_next_hdr = (struct usbh_desc_hdr *)((uint8_t *)p_buf +
						      p_hdr->b_length);
	}
	/* Update buf offset.*/
	*p_offset += p_hdr->b_length;

	return p_next_hdr;
}

/*
 * Format setup request from setup request structure.
 */
static void usbh_fmt_setup_req(struct usbh_setup_req *p_setup_req,
			       void *p_buf_dest)
{
	struct usbh_setup_req *p_buf_dest_setup_req;

	p_buf_dest_setup_req = (struct usbh_setup_req *)p_buf_dest;

	p_buf_dest_setup_req->bm_request_type = p_setup_req->bm_request_type;
	p_buf_dest_setup_req->b_request = p_setup_req->b_request;
	p_buf_dest_setup_req->w_value =
		sys_get_le16((uint8_t *)&p_setup_req->w_value);
	p_buf_dest_setup_req->w_index =
		sys_get_le16((uint8_t *)&p_setup_req->w_index);
	p_buf_dest_setup_req->w_length =
		sys_get_le16((uint8_t *)&p_setup_req->w_length);
}

/*
 * Parse device descriptor into device descriptor structure.
 */
static void usbh_parse_dev_desc(struct usbh_dev_desc *p_dev_desc,
				void *p_buf_src)
{
	struct usbh_dev_desc *p_buf_src_dev_desc;

	p_buf_src_dev_desc = (struct usbh_dev_desc *)p_buf_src;

	p_dev_desc->b_length = p_buf_src_dev_desc->b_length;
	p_dev_desc->b_desc_type = p_buf_src_dev_desc->b_desc_type;
	p_dev_desc->bcd_usb =
		sys_get_le16((uint8_t *)&p_buf_src_dev_desc->bcd_usb);
	p_dev_desc->b_device_class = p_buf_src_dev_desc->b_device_class;
	p_dev_desc->b_device_sub_class = p_buf_src_dev_desc->b_device_sub_class;
	p_dev_desc->b_device_protocol = p_buf_src_dev_desc->b_device_protocol;
	p_dev_desc->b_max_packet_size_zero =
		p_buf_src_dev_desc->b_max_packet_size_zero;
	p_dev_desc->id_vendor =
		sys_get_le16((uint8_t *)&p_buf_src_dev_desc->id_vendor);
	p_dev_desc->id_product =
		sys_get_le16((uint8_t *)&p_buf_src_dev_desc->id_product);
	p_dev_desc->bcd_device =
		sys_get_le16((uint8_t *)&p_buf_src_dev_desc->bcd_device);
	p_dev_desc->i_manufacturer = p_buf_src_dev_desc->i_manufacturer;
	p_dev_desc->i_product = p_buf_src_dev_desc->i_product;
	p_dev_desc->i_serial_number = p_buf_src_dev_desc->i_serial_number;
	p_dev_desc->b_nbr_configs = p_buf_src_dev_desc->b_nbr_configs;
}

/*
 * Parse configuration descriptor into configuration descriptor structure.
 */
static void usbh_parse_cfg_desc(struct usbh_cfg_desc *p_cfg_desc,
				void *p_buf_src)
{
	struct usbh_cfg_desc *p_buf_src_cfg_desc;

	p_buf_src_cfg_desc = (struct usbh_cfg_desc *)p_buf_src;

	p_cfg_desc->b_length = p_buf_src_cfg_desc->b_length;
	p_cfg_desc->b_desc_type = p_buf_src_cfg_desc->b_desc_type;
	p_cfg_desc->w_total_length =
		sys_get_le16((uint8_t *)&p_buf_src_cfg_desc->w_total_length);
	p_cfg_desc->b_nbr_interfaces = p_buf_src_cfg_desc->b_nbr_interfaces;
	p_cfg_desc->b_cfg_value = p_buf_src_cfg_desc->b_cfg_value;
	p_cfg_desc->i_cfg = p_buf_src_cfg_desc->i_cfg;
	p_cfg_desc->bm_attributes = p_buf_src_cfg_desc->bm_attributes;
	p_cfg_desc->b_max_pwr = p_buf_src_cfg_desc->b_max_pwr;
}

/*
 * Parse interface descriptor into interface descriptor structure.
 */
static void usbh_parse_if_desc(struct usbh_if_desc *p_if_desc, void *p_buf_src)
{
	struct usbh_if_desc *p_buf_src_if_desc;

	p_buf_src_if_desc = (struct usbh_if_desc *)p_buf_src;

	p_if_desc->b_length = p_buf_src_if_desc->b_length;
	p_if_desc->b_desc_type = p_buf_src_if_desc->b_desc_type;
	p_if_desc->b_if_nbr = p_buf_src_if_desc->b_if_nbr;
	p_if_desc->b_alt_setting = p_buf_src_if_desc->b_alt_setting;
	p_if_desc->b_nbr_endpoints = p_buf_src_if_desc->b_nbr_endpoints;
	p_if_desc->b_if_class = p_buf_src_if_desc->b_if_class;
	p_if_desc->b_if_sub_class = p_buf_src_if_desc->b_if_sub_class;
	p_if_desc->b_if_protocol = p_buf_src_if_desc->b_if_protocol;
	p_if_desc->i_interface = p_buf_src_if_desc->i_interface;
}

/*
 * Parse endpoint descriptor into endpoint descriptor structure.
 */
static void usbh_parse_ep_desc(struct usbh_ep_desc *p_ep_desc, void *p_buf_src)
{
	struct usbh_ep_desc *p_buf_desc;

	p_buf_desc = (struct usbh_ep_desc *)p_buf_src;
	p_ep_desc->b_length = p_buf_desc->b_length;
	p_ep_desc->b_desc_type = p_buf_desc->b_desc_type;
	p_ep_desc->b_endpoint_address = p_buf_desc->b_endpoint_address;
	p_ep_desc->bm_attributes = p_buf_desc->bm_attributes;
	p_ep_desc->w_max_packet_size =
		sys_get_le16((uint8_t *)&p_buf_desc->w_max_packet_size);
	p_ep_desc->b_interval = p_buf_desc->b_interval;
	/* Following fields only relevant for isoc EPs.         */
	if ((p_ep_desc->bm_attributes & 0x03) == USBH_EP_TYPE_ISOC) {
		p_ep_desc->b_refresh = p_buf_desc->b_refresh;
		p_ep_desc->b_sync_address = p_buf_desc->b_sync_address;
	}
}

/*
 * Task that process asynchronous URBs
 */
static void usbh_async_task(void *p_arg, void *p_arg2, void *p_arg3)
{
	struct usbh_urb *p_urb;
	int key;

	while (true) {
		/* Wait for URBs processed by HC.*/
		k_sem_take(&usbh_urb_sem, K_FOREVER);

		key = irq_lock();
		p_urb = (struct usbh_urb *)usbh_urb_head_ptr;

		if (usbh_urb_head_ptr == usbh_urb_tail_ptr) {
			usbh_urb_head_ptr = NULL;
			usbh_urb_tail_ptr = NULL;
		} else {
			usbh_urb_head_ptr = usbh_urb_head_ptr->nxt_ptr;
		}
		irq_unlock(key);

		if (p_urb != NULL) {
			usbh_urb_complete(p_urb);
		}
	}
}
