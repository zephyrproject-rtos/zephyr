/*
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief NX20P3483 PPC registers definitions
 */

#ifndef ZEPHYR_DRIVERS_USBC_PPC_NXP_NX20P3483_PRIV_H_
#define ZEPHYR_DRIVERS_USBC_PPC_NXP_NX20P3483_PRIV_H_

#include<zephyr/dt-bindings/usb-c/nxp_nx20p3483.h>

/** Register address - device id */
#define NX20P3483_REG_DEVICE_ID 0x00
/** Bit mask for vendor id */
#define NX20P3483_REG_DEVICE_ID_VENDOR_MASK GENMASK(7, 3)
/** Bit mask for version id */
#define NX20P3483_REG_DEVICE_ID_REVISION_MASK GENMASK(2, 0)

/** Register address - device status */
#define NX20P3483_REG_DEVICE_STATUS 0x01
/** Bit mask for device mode */
#define NX20P3483_REG_DEVICE_STATUS_MODE_MASK GENMASK(2, 0)

/** Value for dead battery mode */
#define NX20P3483_MODE_DEAD_BATTERY 0
/** Value for high-voltage sink mode */
#define NX20P3483_MODE_HV_SNK  1
/** Value for 5V source mode */
#define NX20P3483_MODE_5V_SRC  2
/** Value for high-voltage source mode */
#define NX20P3483_MODE_HV_SRC  3
/** Value for standby mode */
#define NX20P3483_MODE_STANDBY 4

/** Register address - switch control */
#define NX20P3483_REG_SWITCH_CTRL       0x02
/** Bit field for source path selection. If set, HV source path is selected, 5V otherwise. */
#define NX20P3483_REG_SWITCH_CTRL_SRC   BIT(7)

/** Register address - switch status */
#define NX20P3483_REG_SWITCH_STATUS               0x03
/** Bit field for 5V source switch enabled */
#define NX20P3483_REG_SWITCH_STATUS_5VSRC         BIT(2)
/** Bit field for HV source switch enabled */
#define NX20P3483_REG_SWITCH_STATUS_HVSRC         BIT(1)
/** Bit field for HV sink switch enabled */
#define NX20P3483_REG_SWITCH_STATUS_HVSNK         BIT(0)

/** Register address - interrupt1 */
#define NX20P3483_REG_INT1            0x04
/** Bit field for exit dead battery error */
#define NX20P3483_REG_INT1_DBEXIT_ERR BIT(7)
/** Bit field for overvoltage fault triggered on 5V source path */
#define NX20P3483_REG_INT1_OV_5VSRC   BIT(4)
/** Bit field for reverse current fault triggered on 5V source path */
#define NX20P3483_REG_INT1_RCP_5VSRC  BIT(3)
/** Bit field for short circuit fault triggered on 5V source path */
#define NX20P3483_REG_INT1_SC_5VSRC   BIT(2)
/** Bit field for overcurrent fault triggered on 5V source path */
#define NX20P3483_REG_INT1_OC_5VSRC   BIT(1)
/** Bit field for over temperature protection fault triggered */
#define NX20P3483_REG_INT1_OTP        BIT(0)

/** Register address - interrupt2*/
#define NX20P3483_REG_INT2           0x05
/** Bit field for sink and source routes enabled fault */
#define NX20P3483_REG_INT2_EN_ERR    BIT(7)
/** Bit field for reverse current fault triggered on HV sink path */
#define NX20P3483_REG_INT2_RCP_HVSNK BIT(6)
/** Bit field for short circuit fault triggered on HV sink path */
#define NX20P3483_REG_INT2_SC_HVSNK  BIT(5)
/** Bit field for overvoltage fault triggered on HV sink path */
#define NX20P3483_REG_INT2_OV_HVSNK  BIT(4)
/** Bit field for reverse current fault triggered on HV source path */
#define NX20P3483_REG_INT2_RCP_HVSRC BIT(3)
/** Bit field for short circuit fault triggered on HV source path */
#define NX20P3483_REG_INT2_SC_HVSRC  BIT(2)
/** Bit field for overcurrent fault triggered on HV source path */
#define NX20P3483_REG_INT2_OC_HVSRC  BIT(1)
/** Bit field for overvoltage fault triggered on HV source path */
#define NX20P3483_REG_INT2_OV_HVSRC  BIT(0)

/** Register address - interrupt1 mask */
#define NX20P3483_REG_INT1_MASK 0x06

/** Register address - interrupt2 mask*/
#define NX20P3483_REG_INT2_MASK 0x07

/** Register address - OVLO threshold (overvoltage threshold) */
#define NX20P3483_REG_OVLO_THRESHOLD      0x08
/**
 * Bit mask for overvoltage threshold value
 * Values used in this register are defined as NX20P3483_U_THRESHOLD_*
 */
#define NX20P3483_REG_OVLO_THRESHOLD_MASK GENMASK(2, 0)

/* Internal 5V VBUS Switch Current Limit Settings (min) */
#define NX20P3483_ILIM_MASK  0xF

/**
 * Register address - HV source switch OCP threshold
 * Values used in this register are defined as NX20P3483_I_THRESHOLD_*
 */
#define NX20P3483_REG_HV_SRC_OCP_THRESHOLD 0x09

/**
 * Register address - 5V source switch OCP threshold
 * Values used in this register are defined as NX20P3483_I_THRESHOLD_*
 */
#define NX20P3483_REG_5V_SRC_OCP_THRESHOLD 0x0A

/** Register address - device control */
#define NX20P3483_REG_DEVICE_CTRL            0x0B
/** Bit field for fast role swap capability activated */
#define NX20P3483_REG_DEVICE_CTRL_FRS_AT     BIT(3)
/** Bit field for exit dead battery mode */
#define NX20P3483_REG_DEVICE_CTRL_DB_EXIT    BIT(2)
/** Bit field for VBUS discharge circuit enabled */
#define NX20P3483_REG_DEVICE_CTRL_VBUSDIS_EN BIT(1)
/** Bit field for LDO shutdown */
#define NX20P3483_REG_DEVICE_CTRL_LDO_SD     BIT(0)

#endif /* ZEPHYR_DRIVERS_USBC_PPC_NXP_NX20P3483_PRIV_H_ */
