/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file is based on usbh_class.c from uC/Modbus Stack.
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

#include <usbh_class.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(usbh_class, CONFIG_USBH_LOG_LEVEL);

struct usbh_class_drv_reg usbh_class_drv_list[USBH_CFG_MAX_NBR_CLASS_DRVS];

static int usbh_class_probe_dev(struct usbh_dev *p_dev);

static int usbh_class_probe_if(struct usbh_dev *p_dev, struct usbh_if *p_if);

static void usbh_class_notify(struct usbh_dev *p_dev, struct usbh_if *p_if,
			      void *p_class_dev, uint8_t is_conn);

/*
 * Registers a class driver to the USB host stack.
 */
int usbh_reg_class_drv(const struct usbh_class_drv *p_class_drv,
		       usbh_class_notify_fnct class_notify_fnct,
		       void *p_class_notify_ctx)
{
	uint8_t ix;
	int err;
	int key;

	if (p_class_drv == NULL) {
		return -EINVAL;
	}

	if (p_class_drv->name_ptr == NULL) {
		return -EINVAL;
	}

	if ((p_class_drv->probe_dev == 0) && (p_class_drv->probe_if == 0)) {
		return -EINVAL;
	}

	key = irq_lock();
	for (ix = 0; ix < USBH_CFG_MAX_NBR_CLASS_DRVS; ix++) {
		/* Find first empty element in the class drv list.*/

		if (usbh_class_drv_list[ix].in_use == 0) {
			/* Insert class drv if it is empty location.*/
			usbh_class_drv_list[ix].class_drv_ptr = p_class_drv;
			usbh_class_drv_list[ix].notify_fnct_ptr =
				class_notify_fnct;
			usbh_class_drv_list[ix].notify_arg_ptr =
				p_class_notify_ctx;
			usbh_class_drv_list[ix].in_use = 1;
			break;
		}
	}
	irq_unlock(key);

	if (ix >= USBH_CFG_MAX_NBR_CLASS_DRVS) {
		/* List is full. */
		return -ERANGE;
	}

	p_class_drv->global_init(&err);

	return err;
}

/*
 * Unregisters a class driver from the USB host stack.
 */
int usbh_class_drv_unreg(const struct usbh_class_drv *p_class_drv)
{
	uint8_t ix;
	int key;

	if (p_class_drv == NULL) {
		return -EINVAL;
	}

	key = irq_lock();
	for (ix = 0; ix < USBH_CFG_MAX_NBR_CLASS_DRVS; ix++) {
		/* Find the element in the class driver list.*/

		if ((usbh_class_drv_list[ix].in_use != 0) &&
		    (usbh_class_drv_list[ix].class_drv_ptr == p_class_drv)) {
			usbh_class_drv_list[ix].class_drv_ptr = NULL;
			usbh_class_drv_list[ix].notify_fnct_ptr = NULL;
			usbh_class_drv_list[ix].notify_arg_ptr = NULL;
			usbh_class_drv_list[ix].in_use = 0;
			break;
		}
	}
	irq_unlock(key);

	if (ix >= USBH_CFG_MAX_NBR_CLASS_DRVS) {
		return -ENOENT;
	}

	return 0;
}

/*
 * suspend all class drivers associated to the device.
 */
void usbh_class_suspend(struct usbh_dev *p_dev)
{
	uint8_t if_ix;
	uint8_t nbr_ifs;
	struct usbh_cfg *p_cfg;
	struct usbh_if *p_if;
	const struct usbh_class_drv *p_class_drv;

	/* If class drv is present at dev level.*/
	if ((p_dev->class_dev_ptr != NULL) &&
	    (p_dev->class_drv_reg_ptr != NULL)) {
		p_class_drv = p_dev->class_drv_reg_ptr->class_drv_ptr;
		/* Chk if class drv is present at dev level.*/
		if ((p_class_drv != NULL) && (p_class_drv->suspend != NULL)) {
			p_class_drv->suspend(p_dev->class_dev_ptr);
			return;
		}
	}
	/* Get first cfg.*/
	p_cfg = usbh_cfg_get(p_dev, 0);
	nbr_ifs = usbh_cfg_if_nbr_get(p_cfg);

	for (if_ix = 0; if_ix < nbr_ifs; if_ix++) {
		p_if = usbu_if_get(p_cfg, if_ix);
		if (p_if == NULL) {
			return;
		}

		if ((p_if->class_dev_ptr != NULL) &&
		    (p_if->class_drv_reg_ptr != NULL)) {
			p_class_drv = p_if->class_drv_reg_ptr->class_drv_ptr;

			if ((p_class_drv != NULL) &&
			    (p_class_drv->suspend != NULL)) {
				p_class_drv->suspend(p_if->class_dev_ptr);
				return;
			}
		}
	}
}

/*
 * resume all class drivers associated to the device.
 */
void usbh_class_resume(struct usbh_dev *p_dev)
{
	uint8_t if_ix;
	uint8_t nbr_ifs;
	struct usbh_cfg *p_cfg;
	struct usbh_if *p_if;
	const struct usbh_class_drv *p_class_drv;

	/* If class drv is present at dev level.*/
	if ((p_dev->class_dev_ptr != NULL) &&
	    (p_dev->class_drv_reg_ptr != NULL)) {
		p_class_drv = p_dev->class_drv_reg_ptr->class_drv_ptr;
		/* Chk if class drv is present at dev level.*/
		if ((p_class_drv != NULL) && (p_class_drv->resume != NULL)) {
			p_class_drv->resume(p_dev->class_dev_ptr);
			return;
		}
	}

	/* Get first cfg.*/
	p_cfg = usbh_cfg_get(p_dev, 0);
	nbr_ifs = usbh_cfg_if_nbr_get(p_cfg);

	for (if_ix = 0; if_ix < nbr_ifs; if_ix++) {
		p_if = usbu_if_get(p_cfg, if_ix);
		if (p_if == NULL) {
			return;
		}

		if ((p_if->class_dev_ptr != NULL) &&
		    (p_if->class_drv_reg_ptr != NULL)) {
			p_class_drv = p_if->class_drv_reg_ptr->class_drv_ptr;

			if ((p_class_drv != NULL) &&
			    (p_class_drv->resume != NULL)) {
				p_class_drv->resume(p_if->class_dev_ptr);
				return;
			}
		}
	}
}

/*
 * Once a device is connected, attempts to find a class driver
 * matching the device descriptor.
 * If no class driver matches the device descriptor, it will
 * attempt to find a class driver for each interface present
 * in the active configuration.
 */
int usbh_class_drv_conn(struct usbh_dev *p_dev)
{
	uint8_t if_ix;
	uint8_t nbr_if;
	bool drv_found;
	int err;
	struct usbh_cfg *p_cfg;
	struct usbh_if *p_if;

	err = usbh_class_probe_dev(p_dev);
	if (err == 0) {
		p_if = NULL;
		/* Find a class drv matching dev desc.*/
		usbh_class_notify(p_dev, p_if, p_dev->class_dev_ptr,
				  USBH_CLASS_DEV_STATE_CONN);
		return 0;
	} else if (err != -ENOTSUP) {
		LOG_ERR("ERROR: Probe class driver. #%d\r\n", err);
	} else {
		/* Empty Else statement*/
	}
	/* Select first cfg.*/
	err = usbh_cfg_set(p_dev, 1);
	if (err != 0) {
		return err;
	}

	drv_found = false;
	/* Get active cfg struct.*/
	p_cfg = usbh_cfg_get(p_dev, (p_dev->sel_cfg - 1));
	nbr_if = usbh_cfg_if_nbr_get(p_cfg);

	for (if_ix = 0; if_ix < nbr_if; if_ix++) {
		/* For all IFs present in cfg.*/
		p_if = usbu_if_get(p_cfg, if_ix);
		if (p_if == NULL) {
			return -ENOTSUP;
		}
		/* Find class driver matching IF.*/
		err = usbh_class_probe_if(p_dev, p_if);
		if (err == 0) {
			drv_found = true;
		} else if (err != -ENOTSUP) {
			LOG_ERR("ERROR: Probe class driver. #%d\r\n", err);
		} else {
			/* Empty Else statement */
		}
	}
	if (drv_found == false) {
		LOG_ERR("No Class Driver Found.\r\n");
		return err;
	}

	for (if_ix = 0; if_ix < nbr_if; if_ix++) {
		/* For all IFs present in this cfg, notify app.*/
		p_if = usbu_if_get(p_cfg, if_ix);
		if (p_if == NULL) {
			return -ENOTSUP;
		}

		if (p_if->class_dev_ptr != 0) {
			usbh_class_notify(p_dev, p_if, p_if->class_dev_ptr,
					  USBH_CLASS_DEV_STATE_CONN);
		}
	}

	return 0;
}

/*
 * disconnect all the class drivers associated to the specified USB device.
 */
void usbh_class_drv_disconn(struct usbh_dev *p_dev)
{
	uint8_t if_ix;
	uint8_t nbr_ifs;
	struct usbh_cfg *p_cfg;
	struct usbh_if *p_if = NULL;
	const struct usbh_class_drv *p_class_drv;

	/* If class drv is present at dev level.*/
	if ((p_dev->class_dev_ptr != NULL) &&
	    (p_dev->class_drv_reg_ptr != NULL)) {
		p_class_drv = p_dev->class_drv_reg_ptr->class_drv_ptr;

		if ((p_class_drv != NULL) && (p_class_drv->disconn != 0)) {
			usbh_class_notify(p_dev, p_if, p_dev->class_dev_ptr,
					  USBH_CLASS_DEV_STATE_DISCONN);

			p_class_drv->disconn((void *)p_dev->class_dev_ptr);
		}

		p_dev->class_drv_reg_ptr = NULL;
		p_dev->class_dev_ptr = NULL;
		return;
	}

	/* Get first cfg.*/
	p_cfg = usbh_cfg_get(p_dev, 0);
	nbr_ifs = usbh_cfg_if_nbr_get(p_cfg);
	for (if_ix = 0; if_ix < nbr_ifs; if_ix++) {
		/* For all IFs present in first cfg.*/
		p_if = usbu_if_get(p_cfg, if_ix);
		if (p_if == NULL) {
			return;
		}

		if ((p_if->class_dev_ptr != NULL) &&
		    (p_if->class_drv_reg_ptr != NULL)) {
			p_class_drv = p_if->class_drv_reg_ptr->class_drv_ptr;

			if ((p_class_drv != NULL) &&
			    (p_class_drv->disconn != NULL)) {
				usbh_class_notify(p_dev, p_if,
						  p_if->class_dev_ptr,
						  USBH_CLASS_DEV_STATE_DISCONN);

				/* disconnect the class driver.*/
				p_class_drv->disconn(
					(void *)p_if->class_dev_ptr);
			}

			p_if->class_drv_reg_ptr = NULL;
			p_if->class_dev_ptr = NULL;
		}
	}
}

/*
 * Find a class driver matching device descriptor of the USB device.
 */
static int usbh_class_probe_dev(struct usbh_dev *p_dev)
{
	uint8_t ix;
	const struct usbh_class_drv *p_class_drv;
	int err;
	void *p_class_dev;

	err = -ENOTSUP;

	for (ix = 0; ix < USBH_CFG_MAX_NBR_CLASS_DRVS; ix++) {
		/* For each class drv present in list.*/

		if (usbh_class_drv_list[ix].in_use != 0) {
			p_class_drv = usbh_class_drv_list[ix].class_drv_ptr;

			if (p_class_drv->probe_dev != NULL) {
				p_dev->class_drv_reg_ptr =
					&usbh_class_drv_list[ix];
				p_class_dev =
					p_class_drv->probe_dev(p_dev, &err);

				if (err == 0) {
					/* Drv found, store class dev ptr.*/
					p_dev->class_dev_ptr = p_class_dev;
					return err;
				}
				p_dev->class_drv_reg_ptr = NULL;
			}
		}
	}

	return err;
}

/*
 * Finds a class driver matching interface descriptor of an interface.
 */
static int usbh_class_probe_if(struct usbh_dev *p_dev, struct usbh_if *p_if)
{
	uint8_t ix;
	const struct usbh_class_drv *p_class_drv;
	void *p_class_dev;
	int err;

	err = -ENOTSUP;
	for (ix = 0; ix < USBH_CFG_MAX_NBR_CLASS_DRVS; ix++) {
		/* Search drv list for matching IF class. */
		if (usbh_class_drv_list[ix].in_use != 0) {
			p_class_drv = usbh_class_drv_list[ix].class_drv_ptr;

			if (p_class_drv->probe_if != NULL) {
				p_if->class_drv_reg_ptr =
					&usbh_class_drv_list[ix];
				p_class_dev = p_class_drv->probe_if(p_dev, p_if,
								    &err);

				if (err == 0) {
					/* Drv found, store class dev ptr.*/
					p_if->class_dev_ptr = p_class_dev;
					return err;
				}
				p_if->class_drv_reg_ptr = NULL;
			}
		}
	}

	return err;
}

/*
 * Description : Notifies application about connection and disconnection events.
 */
static void usbh_class_notify(struct usbh_dev *p_dev, struct usbh_if *p_if,
			      void *p_class_dev, uint8_t is_conn)
{
	struct usbh_class_drv_reg *p_class_drv_reg;

	p_class_drv_reg = p_dev->class_drv_reg_ptr;

	if (p_class_drv_reg == NULL) {
		p_class_drv_reg = p_if->class_drv_reg_ptr;
	}

	if (p_class_drv_reg->notify_fnct_ptr != NULL) {
		/* Call app notification callback fnct.*/
		p_class_drv_reg->notify_fnct_ptr(
			p_class_dev, is_conn, p_class_drv_reg->notify_arg_ptr);
	}
}
