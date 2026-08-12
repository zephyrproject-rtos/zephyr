/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SCSI command opcodes and constants for USB MSC host
 *
 * Opcode values alias @ref scsi_cmd.h. USB-specific allocation lengths and
 * MODE SENSE page codes live here. CBW/CSW wire formats are in
 * @ref usbh_msc_bot.h; transport completion codes in
 * @ref usbh_msc_transport_errno.h.
 *
 * @since 4.3
 */

#ifndef ZEPHYR_INCLUDE_USB_USBH_MSC_SCSI_CMD_H_
#define ZEPHYR_INCLUDE_USB_USBH_MSC_SCSI_CMD_H_

#include <stdint.h>

#include <zephyr/scsi/scsi_cmd.h>

/** @name SCSI Primary Commands used by the MSC host stack */
/**@{*/
/** SCSI INQUIRY opcode (standard 6-byte CDB). */
#define USB_SCSI_INQUIRY_OPCODE        SCSI_OPCODE_INQUIRY
/** Standard INQUIRY CDB length in bytes. */
#define USB_SCSI_INQUIRY_CDB_LEN       6U
/** Standard INQUIRY data length returned by typical USB MSC devices. */
#define USB_SCSI_INQUIRY_DATA_LEN      36U
/** Allocation length byte for a 36-byte standard INQUIRY response. */
#define USB_SCSI_INQUIRY_ALLOC_LEN_STD 0x24U

/** SCSI READ(10) opcode (SBC). */
#define USB_SCSI_READ10           SCSI_OPCODE_READ_10
/** SCSI WRITE(10) opcode (SBC). */
#define USB_SCSI_WRITE10          SCSI_OPCODE_WRITE_10
/** SCSI VERIFY(10) opcode (SBC); BYTCHK=0 means no DATA phase. */
#define USB_SCSI_VERIFY10         SCSI_OPCODE_VERIFY_10
/** VERIFY(10) CDB length in bytes. */
#define USB_SCSI_VERIFY10_CDB_LEN 10U
/** SCSI MODE SENSE(6) opcode. */
#define USB_SCSI_MODE_SENSE6      SCSI_OPCODE_MODE_SENSE_6
/**@}*/

/** @name MODE SENSE(6) page codes and page control */
/**@{*/
/** Return all mode pages supported by the device. */
#define USB_SCSI_MODE_SENSE_PAGE_ALL      0x3FU
/** Page control: current values. */
#define USB_SCSI_MODE_SENSE_PC_CURRENT    0U
/** Page control: changeable values. */
#define USB_SCSI_MODE_SENSE_PC_CHANGEABLE 1U
/** Page control: default values. */
#define USB_SCSI_MODE_SENSE_PC_DEFAULT    2U
/** Page control: saved values. */
#define USB_SCSI_MODE_SENSE_PC_SAVED      3U
/**@}*/

#endif /* ZEPHYR_INCLUDE_USB_USBH_MSC_SCSI_CMD_H_ */
