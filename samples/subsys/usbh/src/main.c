/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <usbh_msc.h>
#include <string.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(usbh_sample);

static uint8_t buf[512];

static void msc_class_notify(void *p_class_dev, uint8_t is_conn, void *p_ctx)
{
	struct usbh_msc_dev *p_msc_dev;
	int usb_err;
	(void)p_ctx;

	LOG_INF("state %d\n", is_conn);
	p_msc_dev = (struct usbh_msc_dev *)p_class_dev;

	if (is_conn == USBH_CLASS_DEV_STATE_CONN) {
		LOG_INF("MSC connected");
		usb_err = usbh_msc_ref_add(p_msc_dev);
		if (usb_err != 0) {
			LOG_INF("Cannot add new MSC reference w/err: %d",
				usb_err);
			return;
		}
	} else {
		LOG_INF("MSC disconn\n");
		usb_err = usbh_msc_ref_rel(p_msc_dev);
		if (usb_err != 0) {
			LOG_INF("Cannot release MSC reference w/err: %d",
				usb_err);
			return;
		}
	}

	int lun = usbh_msc_max_lun_get(p_msc_dev, &usb_err);

	if (usb_err == 0) {
		uint32_t blocks = 0;
		uint32_t blocksize = 0;

		usbh_msc_capacity_rd(p_msc_dev, lun, &blocks, &blocksize);
		LOG_INF("blocks %d blockSize %d", blocks, blocksize);

		if (usbh_msc_init(p_msc_dev, lun) == 0) {
			LOG_INF("msc initialized");
			LOG_INF("unit rdy %d",
				usbh_msc_unit_rdy_test(p_msc_dev, lun,
						       &usb_err));

			int ret = usbh_msc_write(p_msc_dev, lun, 0, 1,
						 blocksize, &buf, &usb_err);

			LOG_INF("written %d bytes", ret);

			uint8_t temp[512];

			ret = usbh_msc_read(p_msc_dev, lun, 0, 1, blocksize,
					    &temp, &usb_err);

			LOG_INF("read %d bytes", ret);

			for (int i = 0; i < 512; i++) {
				if (temp[i] != buf[i]) {
					LOG_INF("write and read buffers are not\
						 equal");
					return;
				}
			}
			LOG_INF("write and read buffers are equal");
		}
	}
}

void main(void)
{
	memset(&buf[0], 1, 512);
	int err = usbh_init();

	LOG_INF("usbh_init return %d", err);

	uint8_t hc_nbr = usbh_hc_add(&err);

	LOG_INF("usbh_hc_add return %d", hc_nbr);

	err = usbh_reg_class_drv(&usbh_msc_class_drv, msc_class_notify, NULL);

	err = usbh_hc_start(hc_nbr);
	if (err != 0) {
		LOG_ERR("Failed to start host controller %d", err);
	}
}
