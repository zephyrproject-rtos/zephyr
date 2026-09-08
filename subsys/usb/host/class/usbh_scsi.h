/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_USB_HOST_CLASS_USBH_SCSI_H_
#define ZEPHYR_SUBSYS_USB_HOST_CLASS_USBH_SCSI_H_

/* SCSI command opcodes, named after SPC and SBC. The names follow the generic
 * SCSI header proposed in-tree so that they can move there unchanged once it
 * lands.
 */
#define SCSI_TST_U_RDY     0x00U
#define SCSI_REQUEST_SENSE 0x03U
#define SCSI_INQUIRY       0x12U
#define SCSI_READ_CAPACITY 0x25U
#define SCSI_READ10        0x28U
#define SCSI_WRITE10       0x2aU

#define SCSI_TST_U_RDY_CDB_LEN     6U
#define SCSI_REQUEST_SENSE_CDB_LEN 6U
#define SCSI_INQUIRY_CDB_LEN       6U
#define SCSI_READ_CAPACITY_CDB_LEN 10U
#define SCSI_RW10_CDB_LEN          10U

#define SCSI_READ_CAPACITY_RESP_LEN 8U

#define SCSI_MAX_CDB_SIZE 16U

#endif /* ZEPHYR_SUBSYS_USB_HOST_CLASS_USBH_SCSI_H_ */
