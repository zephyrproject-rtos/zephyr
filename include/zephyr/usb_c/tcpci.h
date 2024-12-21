/*
 * Copyright 2024 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USB_C_TCPCI_H_
#define ZEPHYR_INCLUDE_USB_C_TCPCI_H_

/**
 * @file
 * @brief Registers and fields definitions for TypeC Port Controller Interface
 *
 * This file contains register addresses, fields and masks used to retrieve specific data from
 * registry values. They may be used by all TCPC drivers compliant to the TCPCI specification.
 * Registers and fields are compliant to the Type-C Port Controller Interface
 * Specification Revision 2.0, Version 1.3.
 */

/** Register address - vendor id */
#define TCPC_REG_VENDOR_ID 0x0

/** Register address - product id */
#define TCPC_REG_PRODUCT_ID 0x2

/** Register address - version of TCPC */
#define TCPC_REG_BCD_DEV 0x4

/** Register address - USB TypeC version */
#define TCPC_REG_TC_REV            0x6
/** Mask for major part of type-c release supported */
#define TCPC_REG_TC_REV_MAJOR_MASK GENMASK(7, 4)
/** Macro to extract the major part of type-c release supported */
#define TCPC_REG_TC_REV_MAJOR(reg) (((reg) & TCPC_REG_TC_REV_MAJOR_MASK) >> 4)
/** Mask for minor part of type-c release supported */
#define TCPC_REG_TC_REV_MINOR_MASK GENMASK(3, 0)
/** Macro to extract the minor part of type-c release supported */
#define TCPC_REG_TC_REV_MINOR(reg) ((reg) & TCPC_REG_TC_REV_MINOR_MASK)

/** Register address - Power delivery revision */
#define TCPC_REG_PD_REV                0x8
/** Mask for major part of USB PD revision supported */
#define TCPC_REG_PD_REV_REV_MAJOR_MASK GENMASK(15, 12)
/** Macro to extract the major part of USB PD revision supported */
#define TCPC_REG_PD_REV_REV_MAJOR(reg) (((reg) & TCPC_REG_PD_REV_VER_REV_MAJOR_MASK) >> 12)
/** Mask for minor part of USB PD revision supported */
#define TCPC_REG_PD_REV_REV_MINOR_MASK GENMASK(11, 8)
/** Macro to extract the minor part of USB PD revision supported */
#define TCPC_REG_PD_REV_REV_MINOR(reg) (((reg) & TCPC_REG_PD_REV_VER_REV_MINOR_MASK) >> 8)
/** Mask for major part of USB PD version supported */
#define TCPC_REG_PD_REV_VER_MAJOR_MASK GENMASK(7, 4)
/** Macro to extract the major part of USB PD version supported */
#define TCPC_REG_PD_REV_VER_MAJOR(reg) (((reg) & TCPC_REG_PD_REV_VER_VER_MAJOR_MASK) >> 4)
/** Mask for minor part of USB PD version supported */
#define TCPC_REG_PD_REV_VER_MINOR_MASK GENMASK(3, 0)
/** Macro to extract the minor part of USB PD version supported */
#define TCPC_REG_PD_REV_VER_MINOR(reg) ((reg) & TCPC_REG_PD_REV_VER_VER_MINOR_MASK)

/** Register address - interface revision and version */
#define TCPC_REG_PD_INT_REV                0xa
/** Mask for major part of USB Port Controller Interface revision supported */
#define TCPC_REG_PD_INT_REV_REV_MAJOR_MASK GENMASK(15, 12)
/** Macro to extract the major part of USB Port Controller Interface revision supported */
#define TCPC_REG_PD_INT_REV_REV_MAJOR(reg) (((reg) & TCPC_REG_PD_REV_VER_REV_MAJOR_MASK) >> 12)
/** Mask for minor part of USB Port Controller Interface revision supported */
#define TCPC_REG_PD_INT_REV_REV_MINOR_MASK GENMASK(11, 8)
/** Macro to extract the minor part of USB Port Controller Interface revision supported */
#define TCPC_REG_PD_INT_REV_REV_MINOR(reg) (((reg) & TCPC_REG_PD_REV_VER_REV_MINOR_MASK) >> 8)
/** Mask for major part of USB Port Controller Interface version supported */
#define TCPC_REG_PD_INT_REV_VER_MAJOR_MASK GENMASK(7, 4)
/** Macro to extract the major part of USB Port Controller Interface version supported */
#define TCPC_REG_PD_INT_REV_VER_MAJOR(reg) (((reg) & TCPC_REG_PD_REV_VER_VER_MAJOR_MASK) >> 4)
/** Mask for minor part of USB Port Controller Interface version supported */
#define TCPC_REG_PD_INT_REV_VER_MINOR_MASK GENMASK(3, 0)
/** Macro to extract the minor part of USB Port Controller Interface version supported */
#define TCPC_REG_PD_INT_REV_VER_MINOR(reg) ((reg) & TCPC_REG_PD_REV_VER_VER_MINOR_MASK)

/** Register address - alert */
#define TCPC_REG_ALERT              0x10
/** Value for clear alert */
#define TCPC_REG_ALERT_NONE         0x0000
/** Value mask for all alert bits */
#define TCPC_REG_ALERT_MASK_ALL     0xffff
/** Bit for vendor defined alert */
#define TCPC_REG_ALERT_VENDOR_DEF   BIT(15)
/** Bit for extended alert */
#define TCPC_REG_ALERT_ALERT_EXT    BIT(14)
/** Bit for extended status alert */
#define TCPC_REG_ALERT_EXT_STATUS   BIT(13)
/** Bit for beginning of data receive */
#define TCPC_REG_ALERT_RX_BEGINNING BIT(12)
/** Bit for vbus disconnection alert */
#define TCPC_REG_ALERT_VBUS_DISCNCT BIT(11)
/** Bit for receive buffer overflow alert */
#define TCPC_REG_ALERT_RX_BUF_OVF   BIT(10)
/** Bit for fault alert */
#define TCPC_REG_ALERT_FAULT        BIT(9)
/** Bit for low vbus alarm */
#define TCPC_REG_ALERT_V_ALARM_LO   BIT(8)
/** Bit for high vbus alarm */
#define TCPC_REG_ALERT_V_ALARM_HI   BIT(7)
/** Bit for transmission success */
#define TCPC_REG_ALERT_TX_SUCCESS   BIT(6)
/** Bit for transmission discard alert */
#define TCPC_REG_ALERT_TX_DISCARDED BIT(5)
/** Bit for transmission fail alert */
#define TCPC_REG_ALERT_TX_FAILED    BIT(4)
/** Bit for received hard reset alert */
#define TCPC_REG_ALERT_RX_HARD_RST  BIT(3)
/** Bit for data received alert */
#define TCPC_REG_ALERT_RX_STATUS    BIT(2)
/** Bit for power status alert */
#define TCPC_REG_ALERT_POWER_STATUS BIT(1)
/** Bit for CC lines status alert */
#define TCPC_REG_ALERT_CC_STATUS    BIT(0)
/** Bits for any of transmission status alert */
#define TCPC_REG_ALERT_TX_COMPLETE                                                                 \
	(TCPC_REG_ALERT_TX_SUCCESS | TCPC_REG_ALERT_TX_DISCARDED | TCPC_REG_ALERT_TX_FAILED)

/**
 * Register address - alert mask
 * The bits for specific masks are on the same positions as for the @see TCPC_REG_ALERT register.
 */
#define TCPC_REG_ALERT_MASK 0x12

/**
 * Register address - power status mask
 * The bits for specific masks are on the same positions as for the @see TCPC_REG_POWER_STATUS
 * register.
 */
#define TCPC_REG_POWER_STATUS_MASK 0x14

/**
 * Register address - fault status mask
 * The bits for specific masks are on the same positions as for the @see TCPC_REG_FAULT_STATUS
 * register.
 */
#define TCPC_REG_FAULT_STATUS_MASK 0x15

/**
 * Register address - extended status mask
 * The bits for specific masks are on the same positions as for the @see TCPC_REG_EXT_STATUS
 * register.
 */
#define TCPC_REG_EXT_STATUS_MASK 0x16

/**
 * Register address - extended alert mask
 * The bits for specific masks are on the same positions as for the @see TCPC_REG_ALERT_EXT
 * register.
 */
#define TCPC_REG_ALERT_EXT_MASK 0x17

/** Register address - configure standard output */
#define TCPC_REG_CONFIG_STD_OUTPUT                   0x18
/** Bit for high impedance outputs */
#define TCPC_REG_CONFIG_STD_OUTPUT_HIGH_Z            BIT(7)
/** Bit for debug accessory connected# */
#define TCPC_REG_CONFIG_STD_OUTPUT_DBG_ACC_CONN_N    BIT(6)
/** Bit for audio accessory connected# */
#define TCPC_REG_CONFIG_STD_OUTPUT_AUDIO_CONN_N      BIT(5)
/** Bit for active cable */
#define TCPC_REG_CONFIG_STD_OUTPUT_ACTIVE_CABLE      BIT(4)
/** Value mask for mux control */
#define TCPC_REG_CONFIG_STD_OUTPUT_MUX_MASK          (3 << 2)
/** Value for mux - no connection */
#define TCPC_REG_CONFIG_STD_OUTPUT_MUX_NONE          (0 << 2)
/** Value for mux - USB3.1 connected */
#define TCPC_REG_CONFIG_STD_OUTPUT_MUX_USB           (1 << 2)
/** Value for mux - DP alternate mode with 4 lanes */
#define TCPC_REG_CONFIG_STD_OUTPUT_MUX_DP            (2 << 2)
/** Value for mux - USB3.1 + DP 0&1 lines */
#define TCPC_REG_CONFIG_STD_OUTPUT_MUX_USB_DP        (3 << 2)
/** Bit for connection present */
#define TCPC_REG_CONFIG_STD_OUTPUT_CONN_PRESENT      BIT(1)
/** Bit for connector orientation */
#define TCPC_REG_CONFIG_STD_OUTPUT_CONNECTOR_FLIPPED BIT(0)

/** Register address - TCPC control */
#define TCPC_REG_TCPC_CTRL                           0x19
/** Bit for SMBus PEC enabled */
#define TCPC_REG_TCPC_CTRL_SMBUS_PEC                 BIT(7)
/** Bit for enabling the alert assertion when a connection is found */
#define TCPC_REG_TCPC_CTRL_EN_LOOK4CONNECTION_ALERT  BIT(6)
/** Bit for watchdog monitoring */
#define TCPC_REG_TCPC_CTRL_WATCHDOG_TIMER            BIT(5)
/** Bit for enable debug accessory control by TCPM */
#define TCPC_REG_TCPC_CTRL_DEBUG_ACC_CONTROL         BIT(4)
/** Mask*/
#define TCPC_REG_TCPC_CTRL_CLOCK_STRETCH_MASK        GENMASK(3, 2)
/** Value for clock stretching disabled */
#define TCPC_REG_TCPC_CTRL_CLOCK_STRETCH_DISABLED    0
/** Value for limited clock stretching enabled */
#define TCPC_REG_TCPC_CTRL_CLOCK_STRETCH_EN_ALWAYS   (2 << 2)
/** Value for clock stretching enabled only when alert is NOT asserted */
#define TCPC_REG_TCPC_CTRL_CLOCK_STRETCH_EN_NO_ALERT (3 << 2)
/** Bit for BIST test mode enabled */
#define TCPC_REG_TCPC_CTRL_BIST_TEST_MODE            BIT(1)
/** Bit for plug orientation and vconn destination */
#define TCPC_REG_TCPC_CTRL_PLUG_ORIENTATION          BIT(0)

/** Register address - role control */
#define TCPC_REG_ROLE_CTRL          0x1a
/** Bit for dual-role port */
#define TCPC_REG_ROLE_CTRL_DRP_MASK BIT(6)
/** Mask to extract the RP value from register value */
#define TCPC_REG_ROLE_CTRL_RP_MASK  GENMASK(5, 4)
/** Mask to extract the CC2 pull value from register value */
#define TCPC_REG_ROLE_CTRL_CC2_MASK GENMASK(3, 2)
/** Mask to extract the CC! pull value from register value */
#define TCPC_REG_ROLE_CTRL_CC1_MASK GENMASK(1, 0)
/** Macro to set the register value from drp, rp and CC lines values */
#define TCPC_REG_ROLE_CTRL_SET(drp, rp, cc1, cc2)                                                  \
	((((drp) << 6) & TCPC_REG_ROLE_CTRL_DRP_MASK) |                                            \
	 (((rp) << 4) & TCPC_REG_ROLE_CTRL_RP_MASK) |                                              \
	 (((cc2) << 2) & TCPC_REG_ROLE_CTRL_CC2_MASK) | ((cc1) & TCPC_REG_ROLE_CTRL_CC1_MASK))
#define TCPC_REG_ROLE_CTRL_DRP(reg) (((reg) & TCPC_REG_ROLE_CTRL_DRP_MASK) >> 6)
/** Macro to extract the enum tc_rp_value from register value */
#define TCPC_REG_ROLE_CTRL_RP(reg)  (((reg) & TCPC_REG_ROLE_CTRL_RP_MASK) >> 4)
/** Macro to extract the enum tc_cc_pull for CC2 from register value */
#define TCPC_REG_ROLE_CTRL_CC2(reg) (((reg) & TCPC_REG_ROLE_CTRL_CC2_MASK) >> 2)
/** Macro to extract the enum tc_cc_pull for CC1 from register value */
#define TCPC_REG_ROLE_CTRL_CC1(reg) ((reg) & TCPC_REG_ROLE_CTRL_CC1_MASK)

/** Register address - fault control */
#define TCPC_REG_FAULT_CTRL                      0x1b
/** Bit for block the standard input signal force off vbus control */
#define TCPC_REG_FAULT_CTRL_VBUS_FORCE_OFF       BIT(4)
/** Bit for disabling the vbus discharge fault detection timer */
#define TCPC_REG_FAULT_CTRL_VBUS_DISCHARGE_FAULT BIT(3)
/** Bit for disabling the vbus over current detection */
#define TCPC_REG_FAULT_CTRL_VBUS_OCP_FAULT_DIS   BIT(2)
/** Bit for disabling the vbus over voltage detection */
#define TCPC_REG_FAULT_CTRL_VBUS_OVP_FAULT_DIS   BIT(1)
/** Bit for disabling the vconn over current detection */
#define TCPC_REG_FAULT_CTRL_VCONN_OCP_FAULT_DIS  BIT(0)

/** Register address - power control */
#define TCPC_REG_POWER_CTRL                           0x1c
/** Bit for fast role swap enable */
#define TCPC_REG_POWER_CTRL_FRS_ENABLE                BIT(7)
/** Bit for disabling the vbus voltage monitoring */
#define TCPC_REG_POWER_CTRL_VBUS_VOL_MONITOR_DIS      BIT(6)
/** Bit for disabling the voltage alarms */
#define TCPC_REG_POWER_CTRL_VOLT_ALARM_DIS            BIT(5)
/** Bit for enabling the automatic vbus discharge based on the vbus voltage */
#define TCPC_REG_POWER_CTRL_AUTO_DISCHARGE_DISCONNECT BIT(4)
/** Bit for enabling the bleed discharge of vbus */
#define TCPC_REG_POWER_CTRL_BLEED_DISCHARGE           BIT(3)
/** Bit for enabling the forced vbus discharge */
#define TCPC_REG_POWER_CTRL_FORCE_DISCHARGE           BIT(2)
/**
 * Bit for enabling the vconn power supported.
 * If set, the TCPC will deliver at least the power indicated in the vconn power supported in
 * device capabilities register to the vconn.
 * If unset, at least 1W of power will be delivered to vconn.
 */
#define TCPC_REG_POWER_CTRL_VCONN_SUPP                BIT(1)
/** Bit for enabling the vconn sourcing to CC line */
#define TCPC_REG_POWER_CTRL_VCONN_EN                  BIT(0)

/** Register address - CC lines status */
#define TCPC_REG_CC_STATUS                 0x1d
/** Bit for active looking for a connection by TCPC, both DRP and sink/source only */
#define TCPC_REG_CC_STATUS_LOOK4CONNECTION BIT(5)
/** Bit for connection result, set if presenting Rd, unset if presenting Rp*/
#define TCPC_REG_CC_STATUS_CONNECT_RESULT  BIT(4)
/** Mask for CC2 line state */
#define TCPC_REG_CC_STATUS_CC2_STATE_MASK  GENMASK(3, 2)
/**
 * Macro to extract the status value of CC2 line. Interpretation of this value depends on the
 * value of CC2 configuration in Role Control register and on the connect result in this register.
 * For value interpretation look at the CC_STATUS Register Definition in the TCPCI specification.
 */
#define TCPC_REG_CC_STATUS_CC2_STATE(reg)  (((reg) & TCPC_REG_CC_STATUS_CC2_STATE_MASK) >> 2)
/** Mask for CC1 line state */
#define TCPC_REG_CC_STATUS_CC1_STATE_MASK  GENMASK(1, 0)
/** Macto to extract the status value of CC1 line. Look at the information about the CC2 macro. */
#define TCPC_REG_CC_STATUS_CC1_STATE(reg)  ((reg) & TCPC_REG_CC_STATUS_CC1_STATE_MASK)

/** Register address - power status */
#define TCPC_REG_POWER_STATUS               0x1e
/** Bit for debug accessory connected */
#define TCPC_REG_POWER_STATUS_DEBUG_ACC_CON BIT(7)
/** Bit for internal initialization in-progress. If set, only registers 00-0F contain valid data. */
#define TCPC_REG_POWER_STATUS_UNINIT        BIT(6)
/** Bit for sourcing high voltage. If set, the voltage sourced is above the vSafe5V. */
#define TCPC_REG_POWER_STATUS_SOURCING_HV   BIT(5)
/** Bit for sourcing vbus. If set, sourcing vbus is enabled. */
#define TCPC_REG_POWER_STATUS_SOURCING_VBUS BIT(4)
/** Bit for vbus detection enabled. */
#define TCPC_REG_POWER_STATUS_VBUS_DET      BIT(3)
/**
 * Bit for vbus present.
 * If set, the vbus shall be  above 4V. If unset, the vbus shall be below 3.5V.
 */
#define TCPC_REG_POWER_STATUS_VBUS_PRES     BIT(2)
/** Bit for vconn present. Set if vconn is present on CC1 or CC2, threshold is fixed at 2.4V. */
#define TCPC_REG_POWER_STATUS_VCONN_PRES    BIT(1)
/** Bit for sinking vbus. If set, the TCPC is sinking vbus to the system load. */
#define TCPC_REG_POWER_STATUS_SINKING_VBUS  BIT(0)

/** Register address - fault status */
#define TCPC_REG_FAULT_STATUS                      0x1f
/** Bit for all registers reset to default */
#define TCPC_REG_FAULT_STATUS_ALL_REGS_RESET       BIT(7)
/** Bit for force vbus off due to external fault */
#define TCPC_REG_FAULT_STATUS_FORCE_OFF_VBUS       BIT(6)
/** Bit for auto discharge failed */
#define TCPC_REG_FAULT_STATUS_AUTO_DISCHARGE_FAIL  BIT(5)
/** Bit for force discharge failed */
#define TCPC_REG_FAULT_STATUS_FORCE_DISCHARGE_FAIL BIT(4)
/** Bit for internal or external vbus over current */
#define TCPC_REG_FAULT_STATUS_VBUS_OVER_CURRENT    BIT(3)
/** Bit for internal or external vbus over voltage */
#define TCPC_REG_FAULT_STATUS_VBUS_OVER_VOLTAGE    BIT(2)
/** Bit for vconn over current */
#define TCPC_REG_FAULT_STATUS_VCONN_OVER_CURRENT   BIT(1)
/** Bit for I2C interface error */
#define TCPC_REG_FAULT_STATUS_I2C_INTERFACE_ERR    BIT(0)

/** Register address - extended status */
#define TCPC_REG_EXT_STATUS        0x20
/** Bit for vbus at vSafe0V. Set when the TCPC detects that VBUS is below 0.8V. */
#define TCPC_REG_EXT_STATUS_SAFE0V BIT(0)

/** Register address - alert extended */
#define TCPC_REG_ALERT_EXT               0x21
/** Bit for timer expired */
#define TCPC_REG_ALERT_EXT_TIMER_EXPIRED BIT(2)
/** Bit for source fast role swap. Set when FRS signal sent due to standard input being low. */
#define TCPC_REG_ALERT_EXT_SRC_FRS       BIT(1)
/** Bit for sink fast role swap. If set, the fast role swap signal was received. */
#define TCPC_REG_ALERT_EXT_SNK_FRS       BIT(0)

/** Register address - command */
#define TCPC_REG_COMMAND                     0x23
/** Value for wake i2c command */
#define TCPC_REG_COMMAND_WAKE_I2C            0x11
/** Value for disable vbus detect command - disable vbus present and vSafe0V detection */
#define TCPC_REG_COMMAND_DISABLE_VBUS_DETECT 0x22
/** Value for enable vbus detect command - enable vbus present and vSafe0V detection */
#define TCPC_REG_COMMAND_ENABLE_VBUS_DETECT  0x33
/** Value for disable sink vbus - disable sinking power over vbus */
#define TCPC_REG_COMMAND_SNK_CTRL_LOW        0x44
/** Value for sink vbus - enable sinking power over vbus and vbus present detection */
#define TCPC_REG_COMMAND_SNK_CTRL_HIGH       0x55
/** Value for disable source vbus - disable sourcing power over vbus */
#define TCPC_REG_COMMAND_SRC_CTRL_LOW        0x66
/** Value for source vbus default voltage - enable sourcing vSafe5V over vbus */
#define TCPC_REG_COMMAND_SRC_CTRL_DEF        0x77
/** Value for source vbus high voltage - enable sourcing high voltage over vbus */
#define TCPC_REG_COMMAND_SRC_CTRL_HV         0x88
/** Value for look for connection - start DRP toggling if DRP role is set */
#define TCPC_REG_COMMAND_LOOK4CONNECTION     0x99
/**
 * Value for rx one more
 * Configure receiver to automatically clear the receive_detect register after sending next GoodCRC.
 */
#define TCPC_REG_COMMAND_RX_ONE_MORE         0xAA
/**
 * Value for send fast role swap signal
 * Send FRS if TCPC is source with FRS enabled in power control register
 */
#define TCPC_REG_COMMAND_SEND_FRS_SIGNAL     0xCC
/** Value for reset transmit buffer - TCPC resets the pointer of transmit buffer to offset 1 */
#define TCPC_REG_COMMAND_RESET_TRANSMIT_BUF  0xDD
/**
 * Value for reset receive buffer
 * If buffer pointer is at 132 or less, it is reset to 1, otherwise it is reset to 133.
 */
#define TCPC_REG_COMMAND_RESET_RECEIVE_BUF   0xEE
/** Value for i2c idle */
#define TCPC_REG_COMMAND_I2CIDLE             0xFF

/** Register address - device capabilities 1 */
#define TCPC_REG_DEV_CAP_1                                 0x24
/** Bit for vbus high voltage target - if set, VBUS_HV_TARGET register is implemented */
#define TCPC_REG_DEV_CAP_1_VBUS_NONDEFAULT_TARGET          BIT(15)
/** Bit for vbus over current reporting - if set, vbus over current is reported by TCPC */
#define TCPC_REG_DEV_CAP_1_VBUS_OCP_REPORTING              BIT(14)
/** Bit for vbus over voltage reporting - if set, vbus over voltage is reported by TCPC */
#define TCPC_REG_DEV_CAP_1_VBUS_OVP_REPORTING              BIT(13)
/** Bit for bleed discharge - if set, bleed discharge is implemented in TCPC */
#define TCPC_REG_DEV_CAP_1_BLEED_DISCHARGE                 BIT(12)
/** Bit for force discharge - if set, force discharge is implemented in TCPC */
#define TCPC_REG_DEV_CAP_1_FORCE_DISCHARGE                 BIT(11)
/**
 * Bit for vbus measurement and alarm capable
 * If set, TCPC supports vbus voltage measurement and vbus voltage alarms
 */
#define TCPC_REG_DEV_CAP_1_VBUS_MEASURE_ALARM_CAPABLE      BIT(10)
/** Mask for source resistor supported */
#define TCPC_REG_DEV_CAP_1_SRC_RESISTOR_MASK               GENMASK(9, 8)
/**
 * Macro to extract the supported source resistors from register value
 * The value can be cast to enum tc_rp_value and value can be treated as highest amperage supported
 * since the TCPC has also to support lower values than specified.
 */
#define TCPC_REG_DEV_CAP_1_SRC_RESISTOR(reg)               \
	(((reg) & TCPC_REG_DEV_CAP_1_SRC_RESISTOR_MASK) >> 8)
/** Value for Rp default only - only default amperage is supported */
#define TCPC_REG_DEV_CAP_1_SRC_RESISTOR_RP_DEF             0
/** Value for Rp 1.5A and default - support for 1.5A and for default amperage*/
#define TCPC_REG_DEV_CAP_1_SRC_RESISTOR_RP_1P5_DEF         1
/** Value for Rp 3.0A, 1.5A and default - support for 3.0A, 1.5A and default amperage */
#define TCPC_REG_DEV_CAP_1_SRC_RESISTOR_RP_3P0_1P5_DEF     2
/** Mask for power roles supported */
#define TCPC_REG_DEV_CAP_1_POWER_ROLE_MASK                 GENMASK(7, 5)
#define TCPC_REG_DEV_CAP_1_POWER_ROLE(reg)                 \
	(((reg) & TCPC_REG_DEV_CAP_1_POWER_ROLE_MASK) >> 5)
/** Value for support both source and sink only (no DRP) */
#define TCPC_REG_DEV_CAP_1_POWER_ROLE_SRC_OR_SNK           0
/** Value for support source role only */
#define TCPC_REG_DEV_CAP_1_POWER_ROLE_SRC                  1
/** Value for support sink role only */
#define TCPC_REG_DEV_CAP_1_POWER_ROLE_SNK                  2
/** Value for support sink role with accessory */
#define TCPC_REG_DEV_CAP_1_POWER_ROLE_SNK_ACC              3
/** Value for support dual-role port only */
#define TCPC_REG_DEV_CAP_1_POWER_ROLE_DRP                  4
/** Value for support source, sink, dual-role port, adapter and cable */
#define TCPC_REG_DEV_CAP_1_POWER_ROLE_SRC_SNK_DRP_ADPT_CBL 5
/** Value for support source, sink and dual-role port */
#define TCPC_REG_DEV_CAP_1_POWER_ROLE_SRC_SNK_DRP          6
/** Bit for debug SOP' and SOP'' support - if set, all SOP* messages are supported */
#define TCPC_REG_DEV_CAP_1_ALL_SOP_STAR_MSGS_SUPPORTED     BIT(4)
/** Bit for source vconn - if set, TCPC is capable of switching the vconn source */
#define TCPC_REG_DEV_CAP_1_SOURCE_VCONN                    BIT(3)
/** Bit for sink vbus - if set, TCPC is capable of controling the sink path to the system load */
#define TCPC_REG_DEV_CAP_1_SINK_VBUS                       BIT(2)
/**
 * Bit for source high voltage vbus.
 * If set, TCPC can control the source high voltage path to vbus
 */
#define TCPC_REG_DEV_CAP_1_SOURCE_HV_VBUS                  BIT(1)
/** Bit for source vbus - if set, TCPC is capable of controlling the source path to vbus */
#define TCPC_REG_DEV_CAP_1_SOURCE_VBUS                     BIT(0)

/** Register address - device capabilities 2 */
#define TCPC_REG_DEV_CAP_2                         0x26
/** Bit for device capabilities 3 support */
#define TCPC_REG_DEV_CAP_2_CAP_3_SUPPORTED         BIT(15)
/** Bit for message disable disconnect */
#define TCPC_REG_DEV_CAP_2_MSG_DISABLE_DISCONNECT  BIT(14)
/** Bit for generic timer support */
#define TCPC_REG_DEV_CAP_2_GENERIC_TIMER           BIT(13)
/**
 * Bit for long message support
 * If set, the TCPC supports up to 264 bytes content of the SOP*.
 * One I2C transaction can write up to 132 bytes.
 * If unset, the TCPC support only 30 bytes content of the SOP* message.
 */
#define TCPC_REG_DEV_CAP_2_LONG_MSG                BIT(12)
/** Bit for SMBus PEC support. If set, SMBus PEC can be enabled in the TCPC control register. */
#define TCPC_REG_DEV_CAP_2_SMBUS_PEC               BIT(11)
/** Bit for source fast-role swap support. If set, TCPC is capable of sending FRS as source. */
#define TCPC_REG_DEV_CAP_2_SRC_FRS                 BIT(10)
/** Bit for sink fast-role swap support. If set, TCPC is capable of sending FRS as sink. */
#define TCPC_REG_DEV_CAP_2_SNK_FRS                 BIT(9)
/** Bit for watchdog timer support. If set, watchdog can be enabled in the TCPC control register. */
#define TCPC_REG_DEV_CAP_2_WATCHDOG_TIMER          BIT(8)
/**
 * Bit for sink disconnect detection.
 * If set, the sink disconnect threshold can be set. Otherwise, the vbus present value from
 * status register will be used to indicate the sink disconnection.
 */
#define TCPC_REG_DEV_CAP_2_SNK_DISC_DET            BIT(7)
/**
 * Bit for stop discharge threshold. If set, the TCPM can set the voltage threshold at which
 * the forced vbus discharge will be disabled, into the vbus stop discharge threshold register.
 */
#define TCPC_REG_DEV_CAP_2_STOP_DISCHARGE_THRESH   BIT(6)
/** Mask for resolution of voltage alarm */
#define TCPC_REG_DEV_CAP_2_VBUS_VOLTAGE_ALARM_MASK GENMASK(5, 4)
/** Macro to extract the voltage alarm resolution from the register value */
#define TCPC_REG_DEV_CAP_2_VBUS_VOLTAGE_ALARM(reg)                                                 \
	(((reg) & TCPC_REG_DEV_CAP_2_VBUS_VOLTAGE_ALARM_MASK) >> 4)
/** Value for 25mV resolution of voltage alarm, all 10 bits of voltage alarm registers are used. */
#define TCPC_REG_DEV_CAP_2_VBUS_VOLTAGE_ALARM_25MV    0
/** Value for 50mV resolution of voltage alarm, only 9 bits of voltage alarm registers are used. */
#define TCPC_REG_DEV_CAP_2_VBUS_VOLTAGE_ALARM_50MV    1
/** Value for 100mV resolution of voltage alarm, only 8 bits of voltage alarm registers are used. */
#define TCPC_REG_DEV_CAP_2_VBUS_VOLTAGE_ALARM_100MV   2
/** Mask for vconn power supported */
#define TCPC_REG_DEV_CAP_2_VCONN_POWER_SUPPORTED_MASK GENMASK(3, 1)
/** Macro to extract the vconn power supported from the register value */
#define TCPC_REG_DEV_CAP_2_VCONN_POWER_SUPPORTED(reg)                                              \
	(((reg) & TCPC_REG_DEV_CAP_2_VCONN_POWER_SUPPORTED_MASK) >> 1)
/** Value for vconn power supported of 1.0W */
#define TCPC_REG_DEV_CAP_2_VCONN_POWER_SUPPORTED_1_0W     0
/** Value for vconn power supported of 1.5W */
#define TCPC_REG_DEV_CAP_2_VCONN_POWER_SUPPORTED_1_5W     1
/** Value for vconn power supported of 2.0W */
#define TCPC_REG_DEV_CAP_2_VCONN_POWER_SUPPORTED_2_0W     2
/** Value for vconn power supported of 3.0W */
#define TCPC_REG_DEV_CAP_2_VCONN_POWER_SUPPORTED_3_0W     3
/** Value for vconn power supported of 4.0W */
#define TCPC_REG_DEV_CAP_2_VCONN_POWER_SUPPORTED_4_0W     4
/** Value for vconn power supported of 5.0W */
#define TCPC_REG_DEV_CAP_2_VCONN_POWER_SUPPORTED_5_0W     5
/** Value for vconn power supported of 6.0W */
#define TCPC_REG_DEV_CAP_2_VCONN_POWER_SUPPORTED_6_0W     6
/** Value for external vconn power supported */
#define TCPC_REG_DEV_CAP_2_VCONN_POWER_SUPPORTED_EXTERNAL 7
/** Bit for vconn overcurrent fault capable - if set, TCPC can detect the vconn over current */
#define TCPC_REG_DEV_CAP_2_VCONN_OVC_FAULT                BIT(0)

/** Register address - standard input capabilities */
#define TCPC_REG_STD_INPUT_CAP                0x28
/** Mask for source fast role swap */
#define TCPC_REG_STD_INPUT_CAP_SRC_FRS_MASK   GENMASK(4, 3)
/** Macro to extract the source fast role swap from register value */
#define TCPC_REG_STD_INPUT_CAP_SRC_FRS(reg)   (((reg) & TCPC_REG_STD_INPUT_CAP_SRC_FRS_MASK) >> 3)
/** Value for no source fast role swap pin present in TCPC */
#define TCPC_REG_STD_INTPU_CAP_SRC_FRS_NONE   0
/** Value for source fast role swap input only pin present in TCPC */
#define TCPC_REG_STD_INTPU_CAP_SRC_FRS_INPUT  1
/** Value for source fast role swap both input and output pin present in TCPC */
#define TCPC_REG_STD_INTPU_CAP_SRC_FRS_BOTH   2
/** Bit for vbus external over voltage fault. If set, input pin is present in TCPC. */
#define TCPC_REG_STD_INPUT_CAP_EXT_OVP        BIT(2)
/** Bit for vbus external over current fault. If set, input pin is present in TCPC. */
#define TCPC_REG_STD_INPUT_CAP_EXT_OCP        BIT(1)
/** Bit for force off vbus present. If set, input pin is present in TCPC. */
#define TCPC_REG_STD_INPUT_CAP_FORCE_OFF_VBUS BIT(0)

/** Register address - standard output capabilities */
#define TCPC_REG_STD_OUTPUT_CAP                  0x29
/** Bit for vbus sink disconnect detect indicator */
#define TCPC_REG_STD_OUTPUT_CAP_SNK_DISC_DET     BIT(7)
/** Bit for debug accessory indicator */
#define TCPC_REG_STD_OUTPUT_CAP_DBG_ACCESSORY    BIT(6)
/** Bit for vbus present monitor */
#define TCPC_REG_STD_OUTPUT_CAP_VBUS_PRESENT_MON BIT(5)
/** Bit for audio adapter accessory indicator */
#define TCPC_REG_STD_OUTPUT_CAP_AUDIO_ACCESSORY  BIT(4)
/** Bit for active cable indicator */
#define TCPC_REG_STD_OUTPUT_CAP_ACTIVE_CABLE     BIT(3)
/** Bit for mux configuration control */
#define TCPC_REG_STD_OUTPUT_CAP_MUX_CFG_CTRL     BIT(2)
/** Bit for connection present */
#define TCPC_REG_STD_OUTPUT_CAP_CONN_PRESENT     BIT(1)
/** Bit for connector orientation */
#define TCPC_REG_STD_OUTPUT_CAP_CONN_ORIENTATION BIT(0)

/** Register address - configure extended 1 */
#define TCPC_REG_CONFIG_EXT_1                0x2A
/**
 * Bit for fr swap bidirectional pin.
 * If set, the bidirectional FR swap pin is configured as standard output signal.
 * If unset, it's configured as standard input signal.
 */
#define TCPC_REG_CONFIG_EXT_1_FRS_SNK_DIR    BIT(1)
/**
 * Bit for standard input source FR swap.
 * If set, blocks the source fast role swap input signal from triggering the sending of
 * fast role swap signal.
 * If unset, allow the input signal to trigger sending the fast role swap signal.
 */
#define TCPC_REG_CONFIG_EXT_1_STD_IN_SRC_FRS BIT(0)

/**
 * Register address - generic timer
 * Available only if generic timer bit is set in device capabilities 2 register.
 * This register is 16-bit wide and has a resolution of 0.1ms.
 */
#define TCPC_REG_GENERIC_TIMER 0x2c

/** Register address - message header info */
#define TCPC_REG_MSG_HDR_INFO                 0x2e
/** Bit for cable plug. If set, the message originated from a cable plug. */
#define TCPC_REG_MSG_HDR_INFO_CABLE_PLUG      BIT(4)
/** Mask for data role */
#define TCPC_REG_MSG_HDR_INFO_DATA_ROLE_MASK  BIT(3)
/** Macro to extract the data role from register value */
#define TCPC_REG_MSG_HDR_INFO_DATA_ROLE(reg)  (((reg) & TCPC_REG_MSG_HDR_INFO_DATA_ROLE_MASK) >> 3)
/** Value for data role set as UFP */
#define TCPC_REG_MSG_HDR_INFO_DATA_ROLE_UFP   0
/** Value for data role set as DFP */
#define TCPC_REG_MSG_HDR_INFO_DATA_ROLE_DFP   1
/** Mask for Power Delivery Specification Revision */
#define TCPC_REG_MSG_HDR_INFO_PD_REV_MASK     GENMASK(2, 1)
/** Macro to extract the Power Delivery Specification Revision from register value */
#define TCPC_REG_MSG_HDR_INFO_PD_REV(reg)     (((reg) & TCPC_REG_MSG_HDR_INFO_PD_REV_MASK) >> 1)
/** Value for Power Delivery Specification Revision 1.0 */
#define TCPC_REG_MSG_HDR_INFO_PD_REV_1_0      0
/** Value for Power Delivery Specification Revision 2.0 */
#define TCPC_REG_MSG_HDR_INFO_PD_REV_2_0      1
/** Value for Power Delivery Specification Revision 3.0 */
#define TCPC_REG_MSG_HDR_INFO_PD_REV_3_0      2
/** Mask for power role */
#define TCPC_REG_MSG_HDR_INFO_POWER_ROLE_MASK BIT(0)
/** Macro to extract the power role from register value */
#define TCPC_REG_MSG_HDR_INFO_POWER_ROLE(reg) ((reg) & TCPC_REG_MSG_HDR_INFO_POWER_ROLE_MASK)
/** Value for power role set as sink */
#define TCPC_REG_MSG_HDR_INFO_POWER_ROLE_SNK  0
/** Value for power role set as source */
#define TCPC_REG_MSG_HDR_INFO_POWER_ROLE_SRC  1
/**
 * Macro to set the register value with pd revision, data and power role from parameter and as
 * non-cable plug
 */
#define TCPC_REG_MSG_HDR_INFO_SET(pd_rev_type, drole, prole)                                       \
	((drole) << 3 | (pd_rev_type << 1) | (prole))
/** Mask for PD revision and power and data role */
#define TCPC_REG_MSG_HDR_INFO_ROLES_MASK (TCPC_REG_MSG_HDR_INFO_SET(3, 1, 1))

/** Register address - receive detect */
#define TCPC_REG_RX_DETECT                        0x2f
/**
 * Bit for message disable disconnect.
 * If set, the TCPC set as sink shall disable the PD message delivery when the SNK.Open state
 * is detected for debounce time specified in specification.
 * If unset, sink TCPC disables the PD message delivery when vbus sink disconnect detected in
 * alert register is asserted.
 */
#define TCPC_REG_RX_DETECT_MSG_DISABLE_DISCONNECT BIT(7)
/** Bit for enable cable reset. If set, TCPC will detect the cable reset signal. */
#define TCPC_REG_RX_DETECT_CABLE_RST              BIT(6)
/** Bit for enable hard reset. If set, TCPC will detect the hard reset signal. */
#define TCPC_REG_RX_DETECT_HRST                   BIT(5)
/** Bit for enable SOP_DBG'' message. If set, TCPC will detect the SOP_DBG'' messages. */
#define TCPC_REG_RX_DETECT_SOPPP_DBG              BIT(4)
/** Bit for enable SOP_DBG' message. If set, TCPC will detect the SOP_DBG' messages. */
#define TCPC_REG_RX_DETECT_SOPP_DBG               BIT(3)
/** Bit for enable SOP'' message. If set, TCPC will detect the SOP'' messages. */
#define TCPC_REG_RX_DETECT_SOPPP                  BIT(2)
/** Bit for enable SOP' message. If set, TCPC will detect the SOP' messages. */
#define TCPC_REG_RX_DETECT_SOPP                   BIT(1)
/** Bit for enable SOP message. If set, TCPC will detect the SOP messages. */
#define TCPC_REG_RX_DETECT_SOP                    BIT(0)
/** Mask for detecting the SOP messages and hard reset signals */
#define TCPC_REG_RX_DETECT_SOP_HRST_MASK          (TCPC_REG_RX_DETECT_SOP | TCPC_REG_RX_DETECT_HRST)
/** Mask for detecting the SOP, SOP' and SOP'' messages and hard reset signals */
#define TCPC_REG_RX_DETECT_SOP_SOPP_SOPPP_HRST_MASK                                                \
	(TCPC_REG_RX_DETECT_SOP | TCPC_REG_RX_DETECT_SOPP | TCPC_REG_RX_DETECT_SOPPP |             \
	 TCPC_REG_RX_DETECT_HRST)

/**
 * Register address - receive buffer (readable byte count, rx buf frame type, rx buf byte x)
 * In TCPC Rev 2.0, the RECEIVE_BUFFER is comprised of three sets of registers:
 * READABLE_BYTE_COUNT, RX_BUF_FRAME_TYPE and RX_BUF_BYTE_x. These registers can
 * only be accessed by reading at a common register address 30h.
 */
#define TCPC_REG_RX_BUFFER 0x30

/** Register address - transmit */
#define TCPC_REG_TRANSMIT                               0x50
/** Macro to set the transmit register with message type and retries count */
#define TCPC_REG_TRANSMIT_SET_WITH_RETRY(retries, type) ((retries) << 4 | (type))
/** Macro to set the transmit register with message type and without retries */
#define TCPC_REG_TRANSMIT_SET_WITHOUT_RETRY(type)       (type)
/** Value for transmit SOP type message */
#define TCPC_REG_TRANSMIT_TYPE_SOP                      0
/** Value for transmit SOP' type message */
#define TCPC_REG_TRANSMIT_TYPE_SOPP                     1
/** Value for transmit SOP'' type message */
#define TCPC_REG_TRANSMIT_TYPE_SOPPP                    2
/** Value for transmit SOP_DBG' type message */
#define TCPC_REG_TRANSMIT_TYPE_SOP_DBG_P                3
/** Value for transmit SOP_DBG'' type message */
#define TCPC_REG_TRANSMIT_TYPE_SOP_DBG_PP               4
/** Value for transmit hard reset signal */
#define TCPC_REG_TRANSMIT_TYPE_HRST                     5
/** Value for transmit cable reset signal */
#define TCPC_REG_TRANSMIT_TYPE_CABLE_RST                6
/** Value for transmit BIST carrier mode 2 */
#define TCPC_REG_TRANSMIT_TYPE_BIST                     7

/**
 * Register address - transmit buffer
 * In TCPC Rev 2.0, the TRANSMIT_BUFFER holds the I2C_WRITE_BYTE_COUNT and the
 * portion of the SOP* USB PD message payload (including the header and/or the
 * data bytes) most recently written by the TCPM in TX_BUF_BYTE_x. TX_BUF_BYTE_x
 * is “hidden” and can only be accessed by writing to register address 51h
 */
#define TCPC_REG_TX_BUFFER 0x51

/** Register address - vbus voltage */
#define TCPC_REG_VBUS_VOLTAGE                   0x70
/** Mask for vbus voltage measurement */
#define TCPC_REG_VBUS_VOLTAGE_MEASUREMENT_MASK  GENMASK(9, 0)
/** Macro to extract the vbus measurement from the register value */
#define TCPC_REG_VBUS_VOLTAGE_MEASUREMENT(reg)  ((reg) & TCPC_REG_VBUS_VOLTAGE_MEASUREMENT_MASK)
/** Mask for scale factor */
#define TCPC_REG_VBUS_VOLTAGE_SCALE_FACTOR_MASK GENMASK(11, 10)
/** Macro to extract the vbus voltage scale from the register value */
#define TCPC_REG_VBUS_VOLTAGE_SCALE(reg)                                                           \
	(1 << (((reg) & TCPC_REG_VBUS_VOLTAGE_SCALE_FACTOR_MASK) >> 10))
/** Resolution of vbus voltage measurement. It's specified as 25mV. */
#define TCPC_REG_VBUS_VOLTAGE_LSB 25
/**
 * Macro to convert the register value into real voltage measurement taking scale
 * factor into account
 */
#define TCPC_REG_VBUS_VOLTAGE_VBUS(x)                                                              \
	(TCPC_REG_VBUS_VOLTAGE_SCALE(x) * TCPC_REG_VBUS_VOLTAGE_MEASUREMENT(x) *                   \
	 TCPC_REG_VBUS_VOLTAGE_LSB)

/** Register address - vbus sink disconnect threshold */
#define TCPC_REG_VBUS_SINK_DISCONNECT_THRESH         0x72
/**
 * Resolution of the value stored in register.
 * Value read from register must be multiplied by this value to get a real voltage in mV.
 * Voltage in mV written to register must be divided by this constant.
 * Specification defines it as 25mV
 */
#define TCPC_REG_VBUS_SINK_DISCONNECT_THRESH_LSB     25
/** Mask for the valid bits of voltage trip point */
#define TCPC_REG_VBUS_SINK_DISCONNECT_THRESH_MASK    GENMASK(11, 0)
/** Default value for vbus sink disconnect threshold */
#define TCPC_REG_VBUS_SINK_DISCONNECT_THRESH_DEFAULT 0x008C /* 3.5 V */

/** Register address - vbus sink disconnect threshold */
#define TCPC_REG_VBUS_STOP_DISCHARGE_THRESH         0x74
/**
 * Resolution of the value stored in register.
 * Value read from register must be multiplied by this value to get a real voltage in mV.
 * Voltage in mV written to register must be divided by this constant.
 * Specification defines it as 25mV.
 */
#define TCPC_REG_VBUS_STOP_DISCHARGE_THRESH_LSB     25
/** Mask for the valid bits of voltage trip point */
#define TCPC_REG_VBUS_STOP_DISCHARGE_THRESH_MASK    GENMASK(11, 0)
/** Default value for vbus stop discharge threshold */
#define TCPC_REG_VBUS_STOP_DISCHARGE_THRESH_DEFAULT 0x0020 /* 0.8 V */

/** Register address - vbus voltage alarm - high */
#define TCPC_REG_VBUS_VOLTAGE_ALARM_HI_CFG      0x76
/**
 * Resolution of the value stored in register.
 * Value read from register must be multiplied by this value to get a real voltage in mV.
 * Voltage in mV written to register must be divided by this constant.
 * Specification defines it as 25mV
 */
#define TCPC_REG_VBUS_VOLTAGE_ALARM_HI_CFG_LSB  25
/** Mask for the valid bits of voltage trip point */
#define TCPC_REG_VBUS_VOLTAGE_ALARM_HI_CFG_MASK GENMASK(11, 0)

/** Register address - vbus voltage alarm - low */
#define TCPC_REG_VBUS_VOLTAGE_ALARM_LO_CFG      0x78
/**
 * Resolution of the value stored in register.
 * Value read from register must be multiplied by this value to get a real voltage in mV.
 * Voltage in mV written to register must be divided by this constant.
 * Specification defines it as 25mV
 */
#define TCPC_REG_VBUS_VOLTAGE_ALARM_LO_CFG_LSB  25
/** Mask for the valid bits of voltage trip point */
#define TCPC_REG_VBUS_VOLTAGE_ALARM_LO_CFG_MASK GENMASK(11, 0)

/**
 * Register address - vbus nondefault target
 * Available only if vbus nondefault target is asserted in device capabilities 1 register.
 * Purpose of this register is to provide value for nondefault voltage over vbus when sending
 * the source vbus nondefault voltage command to command register.
 */
#define TCPC_REG_VBUS_NONDEFAULT_TARGET     0x7a
/**
 * Resolution of the value stored in register.
 * Value read from register must be multiplied by this value to get a real voltage in mV.
 * Voltage in mV written to register must be divided by this constant.
 * Specification defines it as 20mV
 */
#define TCPC_REG_VBUS_NONDEFAULT_TARGET_LSB 20

/** Register address - device capabilities 3 */
#define TCPC_REG_DEV_CAP_3               0x7c
/** Mask for vbus voltage support */
#define TCPC_REG_DEV_CAP_3_VBUS_MAX_MASK GENMASK(2, 0)
/** Macro to extract the vbus voltage support from register value */
#define TCPC_REG_DEV_CAP_3_VBUS_MAX(reg) ((reg) & TCPC_REG_DEV_CAP_3_VBUS_MAX_MASK)
/** Value for nominal voltage supported of 5V */
#define TCPC_REG_DEV_CAP_3_VBUS_MAX_5V   0
/** Value for nominal voltage supported of 9V */
#define TCPC_REG_DEV_CAP_3_VBUS_MAX_9V   1
/** Value for nominal voltage supported of 15V */
#define TCPC_REG_DEV_CAP_3_VBUS_MAX_15V  2
/** Value for nominal voltage supported of 20V */
#define TCPC_REG_DEV_CAP_3_VBUS_MAX_20V  3
/** Value for nominal voltage supported of 28V */
#define TCPC_REG_DEV_CAP_3_VBUS_MAX_28V  4
/** Value for nominal voltage supported of 36V */
#define TCPC_REG_DEV_CAP_3_VBUS_MAX_36V  5
/** Value for nominal voltage supported of 48V */
#define TCPC_REG_DEV_CAP_3_VBUS_MAX_48V  6

#endif /* ZEPHYR_INCLUDE_USB_C_TCPCI_H_ */
