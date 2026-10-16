/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup mfd_max20356_dt
 * @brief Devicetree helper macros for the MAX20356/MAX20358 PMIC.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_MFD_MAX20356_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_MFD_MAX20356_H_

/**
 * @defgroup mfd_max20356_dt MAX20356/MAX20358 Devicetree helpers
 * @brief Analog Devices MAX20356/MAX20358 PMIC Devicetree helpers
 * @ingroup devicetree
 * @ingroup mfd_interfaces
 * @{
 */

/**
 * @name Regulator operating modes
 *
 * Values for the regulator @c regulator-initial-mode and
 * @c regulator-allowed-modes properties on the LDO1/LDO2 child nodes, which can
 * act either as a linear regulator or as a load switch (LDO(n)Cfg.LDO(n)Mode).
 * @{
 */
/** Linear-regulator (LDO) mode */
#define MAX20356_MODE_LDO         0
/** Load-switch mode */
#define MAX20356_MODE_LOAD_SWITCH 1
/** @} */

/**
 * @name MPC-pin enable-routing selectors
 *
 * Cell values for the @c adi,mpc-enable-map property on the regulators node.
 * Each cell (indexed by MPC pin 0..7) selects the rail whose enable that pin
 * controls ((rail)Ctr.(rail)MPC(pin)), or MAX20356_MPC_NONE to leave the pin
 * unrouted.
 * @{
 */
/** MPC pin not routed to any rail */
#define MAX20356_MPC_NONE      0
/** Route to BUCK1 */
#define MAX20356_MPC_BUCK1     1
/** Route to BUCK2 */
#define MAX20356_MPC_BUCK2     2
/** Route to BUCK3 */
#define MAX20356_MPC_BUCK3     3
/** Route to the buck-boost */
#define MAX20356_MPC_BUCKBOOST 4
/** Route to LDO1 */
#define MAX20356_MPC_LDO1      5
/** Route to LDO2 */
#define MAX20356_MPC_LDO2      6
/** Route to LDO3 */
#define MAX20356_MPC_LDO3      7
/** Route to LDO4 */
#define MAX20356_MPC_LDO4      8
/** Route to LSW1 */
#define MAX20356_MPC_LSW1      9
/** Route to LSW2 */
#define MAX20356_MPC_LSW2      10
/** Route to LSW3 */
#define MAX20356_MPC_LSW3      11
/** @} */

/**
 * @name Power-reset configurations
 *
 * Values for the @c adi,power-reset-config property (BootCfg.PwrRstCfg[3:0]),
 * selecting how PFN1/PFN2 turn the device on, off, and reset it.
 * @{
 */
/** On/off mode, PFN1 active-high on/off, PFN2 active-low soft-reset */
#define MAX20356_PWRRSTCFG_ONOFF_PFN1_HIGH 0
/** On/off mode, PFN1 active-low on/off, PFN2 active-low soft-reset */
#define MAX20356_PWRRSTCFG_ONOFF_PFN1_LOW  1
/** Always-on, PFN1/PFN2 rising edge hard-/soft-reset */
#define MAX20356_PWRRSTCFG_AON_RISING      2
/** Always-on, PFN1/PFN2 falling edge hard-/soft-reset */
#define MAX20356_PWRRSTCFG_AON_FALLING     3
/** Always-on, PFN_ held high during CHGIN insertion resets */
#define MAX20356_PWRRSTCFG_CR_HIGH         4
/** Always-on, PFN_ held low during CHGIN insertion resets */
#define MAX20356_PWRRSTCFG_CR_LOW          5
/** On/off through KIN key presses (PFN1 KIN, PFN2 KOUT) */
#define MAX20356_PWRRSTCFG_KIN             6
/** On/reset through KIN key presses (PFN1 KIN, PFN2 KOUT) */
#define MAX20356_PWRRSTCFG_CSR1            7
/** On/reset through KIN key presses (PFN1 KIN, PFN2 soft-reset) */
#define MAX20356_PWRRSTCFG_CSR2            8
/** Custom always-on, PFN_ held high during CHGIN insertion resets */
#define MAX20356_PWRRSTCFG_CUSTOM_CR_HIGH  9
/** Custom always-on, PFN_ held low during CHGIN insertion resets */
#define MAX20356_PWRRSTCFG_CUSTOM_CR_LOW   10
/** On/off through KIN key presses, with off/seal */
#define MAX20356_PWRRSTCFG_KIN_SEAL        11
/** Custom always-on with off/seal, PFN_ held during CHGIN insertion resets */
#define MAX20356_PWRRSTCFG_CUSTOM_CR_SEAL  12
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_MFD_MAX20356_H_ */
