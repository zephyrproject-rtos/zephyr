/*
 * Copyright 2004-2020 Silicon Laboratories Inc. www.silabs.com
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_USBH_MSC_CLASS_H_
#define ZEPHYR_USBH_MSC_CLASS_H_

#include <usbh_class.h>

#define USBH_MSC_TIMEOUT 10000u

#define USBH_MSC_DEV_NOT_IN_USE 0u
#define USBH_MSC_DEV_IN_USE 1u

#define USBH_MSC_DATA_DIR_IN 0x80u
#define USBH_MSC_DATA_DIR_OUT 0x00u
#define USBH_MSC_DATA_DIR_NONE 0x01u

/*  MSC DEVICE  */
struct usbh_msc_dev {
	struct usbh_ep BulkInEP;        /* Bulk IN  endpoint.*/
	struct usbh_ep BulkOutEP;       /* Bulk OUT endpoint.*/
	struct usbh_dev *dev_ptr;       /* Pointer to USB device.*/
	struct usbh_if *if_ptr;         /* Pointer to interface.*/
	uint8_t state;                  /* state of MSC device.*/
	uint8_t ref_cnt;                /* Cnt of app ref on this dev.*/
	struct k_mutex HMutex;
};

struct msc_inquiry_info {
	uint8_t DevType;
	uint8_t IsRemovable;
	uint8_t Vendor_ID[8];
	uint8_t Product_ID[16];
	uint32_t ProductRevisionLevel;
};

extern const struct usbh_class_drv usbh_msc_class_drv;

int usbh_msc_init(struct usbh_msc_dev *p_msc_dev, uint8_t lun);

uint8_t usbh_msc_max_lun_get(struct usbh_msc_dev *p_msc_dev, int *p_err);

bool usbh_msc_unit_rdy_test(struct usbh_msc_dev *p_msc_dev, uint8_t lun,
			    int *p_err);

int usbh_msc_capacity_rd(struct usbh_msc_dev *p_msc_dev, uint8_t lun,
			 uint32_t *p_nbr_blks, uint32_t *p_blk_size);

int usbh_msc_std_inquiry(struct usbh_msc_dev *p_msc_dev,
			 struct msc_inquiry_info *p_msc_inquiry_info,
			 uint8_t lun);

int usbh_msc_ref_add(struct usbh_msc_dev *p_msc_dev);

int usbh_msc_ref_rel(struct usbh_msc_dev *p_msc_dev);

uint32_t usbh_msc_read(struct usbh_msc_dev *p_msc_dev, uint8_t lun,
		       uint32_t blk_addr, uint16_t nbr_blks, uint32_t blk_size,
		       void *p_arg, int *p_err);

uint32_t usbh_msc_write(struct usbh_msc_dev *p_msc_dev, uint8_t lun,
			uint32_t blk_addr, uint16_t nbr_blks, uint32_t blk_size,
			const void *p_arg, int *p_err);

#endif
