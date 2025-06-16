/*
 * Copyright (c) 2025 Applied Materials
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ANX7327_REG_H__
#define __ANX7327_REG_H__

/* register defs in ANX7327 Register Specification AA-004989-RS */
/* other register defs are in ANX74xx Programming Guide */

#define ANX7327_REMS_READ_EN	3 // Bit position for REMS read enable.
#define ANX7327_RDID_READ_EN	4 // Bit position for RDID read enable.

/* REGISTERS AT 0x58 I2C ADDRESS */
#define ANX7327_PWR_STATUS            0x1E
#define ANX7327_CC_STATUS_REG         0x1D
#define ANX7327_TCPC_VENDOR_ID0_REG   0x00
#define ANX7327_TCPC_VENDOR_ID1_REG   0x01
#define ANX7327_TCPC_PRODUCT_ID0_REG  0x02
#define ANX7327_TCPC_PRODUCT_ID1_REG  0x03
#define ANX7327_TCPC_DEVICE_ID0_REG   0x04
#define ANX7327_TCPC_DEVICE_ID1_REG   0x05
#define ANX7327_ALERT0_REG	      0x10
#define ANX7327_ALERT1_REG	      0x11
#define ANX7327_FAULT_REG	      0x1F

/* REGISTERS AT 0x7E I2C ADDRESS */
#define ANX7327_REMS_REG	      0x30
#define ANX7327_DEV_ID_REG	      0x37
#define ANX7327_MANF_ID_REG	      0x36
#define ANX7327_HPD_CTL0_REG	      0x7E
#define ANX7327_USBC_STATUS_REG	      0xB8
#define ANX7327_ADDR_INTP_SRC0_REG    0x67
#define ANX7327_ADDR_INTP_SRC1_REG    0x68

#endif
