/*
 * SPDX-FileCopyrightText: 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MONITOR_NXP_FREQME_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MONITOR_NXP_FREQME_H_

/**
 * @file
 * @brief INPUTMUX connection cookies for NXP FREQME reference / target routing.
 *
 * The numeric values mirror the inputmux_connection_t enum in each device's
 * MCUX HAL header fsl_inputmux_connections.h:
 *
 *   cookie = selector + (destination_register << PMUX_SHIFT)
 *
 * PMUX_SHIFT is 20 on every FREQME-capable SoC. The destination register and
 * the per-source selector numbers depend on the SoC family, so the cookies are
 * grouped by family below and each group lists the devices it applies to. The
 * device tree and driver simply attach the cookie; the hardware routes it to
 * the encoded register.
 */

/** Bit position of the destination mux register within a connection cookie. */
#define NXP_FREQME_PMUX_SHIFT 20

/**
 * @brief set_source() sentinel: leave that axis (reference or target) unchanged.
 *
 * A real INPUTMUX connection cookie is always nonzero: it always carries a
 * destination-register field (0x180/0x184 on MCX, 0x700/0x704 on RT7xx) in its
 * upper bits, so 0 is free to mean "no change" for the NXP FREQME
 * clock_monitor_set_source() back-end. The public clock_monitor API does not
 * define a generic sentinel; this constant is FREQME-private.
 */
#define NXP_FREQME_SOURCE_UNCHANGED 0U

/*
 * MCX families with separate FREQMEAS_REF (0x180) / FREQMEAS_TAR (0x184) mux
 * registers. MCXA, MCXC, MCXN, MCXL and MCXW all share this register layout;
 * only the per-source selector numbers differ, so each family has its own
 * cookie set below.
 */

/** FREQMEAS reference mux register offset (MCXA/MCXC/MCXN/MCXL/MCXW). */
#define NXP_FREQME_REF_REG 0x180
/** FREQMEAS target mux register offset (MCXA/MCXC/MCXN/MCXL/MCXW). */
#define NXP_FREQME_TAR_REG 0x184

/** Build an MCX reference-route cookie from selector @p sel. */
#define NXP_FREQME_REF(sel) ((sel) + (NXP_FREQME_REF_REG << NXP_FREQME_PMUX_SHIFT))
/** Build an MCX target-route cookie from selector @p sel. */
#define NXP_FREQME_TAR(sel) ((sel) + (NXP_FREQME_TAR_REG << NXP_FREQME_PMUX_SHIFT))

/*
 * MCXA selectors.
 * Devices: MCXA153, MCXA156, MCXA174, MCXA266, MCXA344, MCXA346, MCXA577 and
 *          MCXC162 (identical selector numbering).
 */
#define NXP_FREQME_REF_CLK_IN      NXP_FREQME_REF(1) /**< MCXA/MCXC reference: CLK_IN. */
#define NXP_FREQME_REF_FRO_12M     NXP_FREQME_REF(2) /**< MCXA/MCXC reference: 12 MHz FRO. */
#define NXP_FREQME_REF_FRO_HF_DIV  NXP_FREQME_REF(3) /**< MCXA/MCXC reference: FRO_HF divided. */
#define NXP_FREQME_REF_CLK_16K1    NXP_FREQME_REF(5) /**< MCXA/MCXC reference: 16 kHz CLK_16K1. */
#define NXP_FREQME_REF_SLOW_CLK    NXP_FREQME_REF(6) /**< MCXA/MCXC reference: slow clock. */
#define NXP_FREQME_REF_CLK_IN0     NXP_FREQME_REF(7) /**< MCXA/MCXC reference: CLK_IN0. */
#define NXP_FREQME_REF_CLK_IN1     NXP_FREQME_REF(8) /**< MCXA/MCXC reference: CLK_IN1. */

#define NXP_FREQME_TAR_CLK_IN      NXP_FREQME_TAR(1) /**< MCXA/MCXC target: CLK_IN. */
#define NXP_FREQME_TAR_FRO_12M     NXP_FREQME_TAR(2) /**< MCXA/MCXC target: 12 MHz FRO. */
#define NXP_FREQME_TAR_FRO_HF_DIV  NXP_FREQME_TAR(3) /**< MCXA/MCXC target: FRO_HF divided. */
#define NXP_FREQME_TAR_CLK_16K1    NXP_FREQME_TAR(5) /**< MCXA/MCXC target: 16 kHz CLK_16K1. */
#define NXP_FREQME_TAR_SLOW_CLK    NXP_FREQME_TAR(6) /**< MCXA/MCXC target: slow clock. */
#define NXP_FREQME_TAR_CLK_IN0     NXP_FREQME_TAR(7) /**< MCXA/MCXC target: CLK_IN0. */
#define NXP_FREQME_TAR_CLK_IN1     NXP_FREQME_TAR(8) /**< MCXA/MCXC target: CLK_IN1. */

/*
 * MCXN selectors.
 * Devices: MCXN236, MCXN947.
 */
#define NXP_FREQME_MCXN_REF_CLK_IN       NXP_FREQME_REF(0) /**< MCXN reference: CLK_IN. */
#define NXP_FREQME_MCXN_REF_FRO_12M      NXP_FREQME_REF(1) /**< MCXN reference: 12 MHz FRO. */
#define NXP_FREQME_MCXN_REF_FRO_HF       NXP_FREQME_REF(2) /**< MCXN reference: FRO_HF. */
#define NXP_FREQME_MCXN_REF_OSC32K       NXP_FREQME_REF(4) /**< MCXN reference: 32 kHz OSC. */
#define NXP_FREQME_MCXN_REF_CPU_AHB_CLK  NXP_FREQME_REF(5) /**< MCXN reference: CPU/AHB clock. */
#define NXP_FREQME_MCXN_REF_CLK_IN0      NXP_FREQME_REF(6) /**< MCXN reference: CLK_IN0. */
#define NXP_FREQME_MCXN_REF_CLK_IN1      NXP_FREQME_REF(7) /**< MCXN reference: CLK_IN1. */

#define NXP_FREQME_MCXN_TAR_CLK_IN       NXP_FREQME_TAR(0) /**< MCXN target: CLK_IN. */
#define NXP_FREQME_MCXN_TAR_FRO_12M      NXP_FREQME_TAR(1) /**< MCXN target: 12 MHz FRO. */
#define NXP_FREQME_MCXN_TAR_FRO_HF       NXP_FREQME_TAR(2) /**< MCXN target: FRO_HF. */
#define NXP_FREQME_MCXN_TAR_OSC32K       NXP_FREQME_TAR(4) /**< MCXN target: 32 kHz OSC. */
#define NXP_FREQME_MCXN_TAR_CPU_AHB_CLK  NXP_FREQME_TAR(5) /**< MCXN target: CPU/AHB clock. */
#define NXP_FREQME_MCXN_TAR_CLK_IN0      NXP_FREQME_TAR(6) /**< MCXN target: CLK_IN0. */
#define NXP_FREQME_MCXN_TAR_CLK_IN1      NXP_FREQME_TAR(7) /**< MCXN target: CLK_IN1. */

/*
 * MCXL selectors.
 * Devices: MCXL255.
 */
#define NXP_FREQME_MCXL_REF_FRO_12M     NXP_FREQME_REF(2) /**< MCXL reference: 12 MHz FRO. */
#define NXP_FREQME_MCXL_REF_FRO_HF_DIV  NXP_FREQME_REF(3) /**< MCXL reference: FRO_HF divided. */
#define NXP_FREQME_MCXL_REF_XTAL32K     NXP_FREQME_REF(4) /**< MCXL reference: 32 kHz crystal. */
#define NXP_FREQME_MCXL_REF_CLK_16K0    NXP_FREQME_REF(5) /**< MCXL reference: 16 kHz CLK_16K0. */
#define NXP_FREQME_MCXL_REF_SLOW_CLK    NXP_FREQME_REF(6) /**< MCXL reference: slow clock. */
#define NXP_FREQME_MCXL_REF_CLK_IN0     NXP_FREQME_REF(7) /**< MCXL reference: CLK_IN0. */
#define NXP_FREQME_MCXL_REF_CLK_IN1     NXP_FREQME_REF(8) /**< MCXL reference: CLK_IN1. */

#define NXP_FREQME_MCXL_TAR_FRO_12M     NXP_FREQME_TAR(2) /**< MCXL target: 12 MHz FRO. */
#define NXP_FREQME_MCXL_TAR_FRO_HF_DIV  NXP_FREQME_TAR(3) /**< MCXL target: FRO_HF divided. */
#define NXP_FREQME_MCXL_TAR_XTAL32K     NXP_FREQME_TAR(4) /**< MCXL target: 32 kHz crystal. */
#define NXP_FREQME_MCXL_TAR_CLK_16K0    NXP_FREQME_TAR(5) /**< MCXL target: 16 kHz CLK_16K0. */
#define NXP_FREQME_MCXL_TAR_SLOW_CLK    NXP_FREQME_TAR(6) /**< MCXL target: slow clock. */
#define NXP_FREQME_MCXL_TAR_CLK_IN0     NXP_FREQME_TAR(7) /**< MCXL target: CLK_IN0. */
#define NXP_FREQME_MCXL_TAR_CLK_IN1     NXP_FREQME_TAR(8) /**< MCXL target: CLK_IN1. */

/*
 * MCXW selectors.
 * Devices: MCXW236. A completely distinct selector set.
 */
#define NXP_FREQME_MCXW_REF_EXTERN_OSC   NXP_FREQME_REF(0)  /**< MCXW reference: external OSC. */
#define NXP_FREQME_MCXW_REF_FRO_12M      NXP_FREQME_REF(1)  /**< MCXW reference: 12 MHz FRO. */
#define NXP_FREQME_MCXW_REF_FRO_32M      NXP_FREQME_REF(2)  /**< MCXW reference: 32 MHz FRO. */
#define NXP_FREQME_MCXW_REF_FRO_1M       NXP_FREQME_REF(3)  /**< MCXW reference: 1 MHz FRO. */
#define NXP_FREQME_MCXW_REF_OSC32K       NXP_FREQME_REF(4)  /**< MCXW reference: 32 kHz OSC. */
#define NXP_FREQME_MCXW_REF_MAIN_CLK     NXP_FREQME_REF(5)  /**< MCXW reference: main clock. */
#define NXP_FREQME_MCXW_REF_SPIFI_CLK    NXP_FREQME_REF(6)  /**< MCXW reference: SPIFI clock. */
#define NXP_FREQME_MCXW_REF_PORT0_PIN3   NXP_FREQME_REF(7)  /**< MCXW reference: PORT0 pin 3. */
#define NXP_FREQME_MCXW_REF_PORT0_PIN11  NXP_FREQME_REF(8)  /**< MCXW reference: PORT0 pin 11. */
#define NXP_FREQME_MCXW_REF_PORT0_PIN12  NXP_FREQME_REF(9)  /**< MCXW reference: PORT0 pin 12. */
#define NXP_FREQME_MCXW_REF_PORT0_PIN14  NXP_FREQME_REF(10) /**< MCXW reference: PORT0 pin 14. */

#define NXP_FREQME_MCXW_TAR_EXTERN_OSC   NXP_FREQME_TAR(0)  /**< MCXW target: external OSC. */
#define NXP_FREQME_MCXW_TAR_FRO_12M      NXP_FREQME_TAR(1)  /**< MCXW target: 12 MHz FRO. */
#define NXP_FREQME_MCXW_TAR_FRO_32M      NXP_FREQME_TAR(2)  /**< MCXW target: 32 MHz FRO. */
#define NXP_FREQME_MCXW_TAR_FRO_1M       NXP_FREQME_TAR(3)  /**< MCXW target: 1 MHz FRO. */
#define NXP_FREQME_MCXW_TAR_OSC32K       NXP_FREQME_TAR(4)  /**< MCXW target: 32 kHz OSC. */
#define NXP_FREQME_MCXW_TAR_MAIN_CLK     NXP_FREQME_TAR(5)  /**< MCXW target: main clock. */
#define NXP_FREQME_MCXW_TAR_SPIFI_CLK    NXP_FREQME_TAR(6)  /**< MCXW target: SPIFI clock. */
#define NXP_FREQME_MCXW_TAR_PORT0_PIN3   NXP_FREQME_TAR(7)  /**< MCXW target: PORT0 pin 3. */
#define NXP_FREQME_MCXW_TAR_PORT0_PIN11  NXP_FREQME_TAR(8)  /**< MCXW target: PORT0 pin 11. */
#define NXP_FREQME_MCXW_TAR_PORT0_PIN12  NXP_FREQME_TAR(9)  /**< MCXW target: PORT0 pin 12. */
#define NXP_FREQME_MCXW_TAR_PORT0_PIN14  NXP_FREQME_TAR(10) /**< MCXW target: PORT0 pin 14. */

/*
 * i.MX RT700 selectors.
 * Devices: MIMXRT798S (and RT7xx family).
 *
 * RT700 places the FREQMEAS reference / target mux registers at 0x700 / 0x704
 * (not 0x180 / 0x184 like the MCX families), so it needs its own register
 * constants and helpers.
 */

/** FREQMEAS reference mux register offset (i.MX RT7xx). */
#define NXP_FREQME_RT7XX_REF_REG 0x700
/** FREQMEAS target mux register offset (i.MX RT7xx). */
#define NXP_FREQME_RT7XX_TAR_REG 0x704

/** Build an RT7xx reference-route cookie from selector @p sel. */
#define NXP_FREQME_RT7XX_REF(sel) ((sel) + (NXP_FREQME_RT7XX_REF_REG << NXP_FREQME_PMUX_SHIFT))
/** Build an RT7xx target-route cookie from selector @p sel. */
#define NXP_FREQME_RT7XX_TAR(sel) ((sel) + (NXP_FREQME_RT7XX_TAR_REG << NXP_FREQME_PMUX_SHIFT))

#define NXP_FREQME_RT7XX_REF_OSC_CLK     NXP_FREQME_RT7XX_REF(0) /**< RT7xx ref: system OSC. */
#define NXP_FREQME_RT7XX_REF_FRO1_DIV8   NXP_FREQME_RT7XX_REF(1) /**< RT7xx ref: FRO1 / 8. */
#define NXP_FREQME_RT7XX_REF_FRO1        NXP_FREQME_RT7XX_REF(2) /**< RT7xx ref: FRO1. */
#define NXP_FREQME_RT7XX_REF_LPOSC       NXP_FREQME_RT7XX_REF(3) /**< RT7xx ref: low-power OSC. */
#define NXP_FREQME_RT7XX_REF_OSC32K      NXP_FREQME_RT7XX_REF(4) /**< RT7xx ref: 32 kHz OSC. */
#define NXP_FREQME_RT7XX_REF_FRO0        NXP_FREQME_RT7XX_REF(6) /**< RT7xx ref: FRO0. */
#define NXP_FREQME_RT7XX_REF_FRO2        NXP_FREQME_RT7XX_REF(7) /**< RT7xx ref: FRO2. */
#define NXP_FREQME_RT7XX_REF_GPIO_A_CLK  NXP_FREQME_RT7XX_REF(8) /**< RT7xx ref: GPIO A clock. */
#define NXP_FREQME_RT7XX_REF_GPIO_B_CLK  NXP_FREQME_RT7XX_REF(9) /**< RT7xx ref: GPIO B clock. */

#define NXP_FREQME_RT7XX_TAR_OSC_CLK     NXP_FREQME_RT7XX_TAR(0) /**< RT7xx tar: system OSC. */
#define NXP_FREQME_RT7XX_TAR_FRO1_DIV8   NXP_FREQME_RT7XX_TAR(1) /**< RT7xx tar: FRO1 / 8. */
#define NXP_FREQME_RT7XX_TAR_FRO1        NXP_FREQME_RT7XX_TAR(2) /**< RT7xx tar: FRO1. */
#define NXP_FREQME_RT7XX_TAR_LPOSC       NXP_FREQME_RT7XX_TAR(3) /**< RT7xx tar: low-power OSC. */
#define NXP_FREQME_RT7XX_TAR_OSC32K      NXP_FREQME_RT7XX_TAR(4) /**< RT7xx tar: 32 kHz OSC. */
#define NXP_FREQME_RT7XX_TAR_FRO0        NXP_FREQME_RT7XX_TAR(6) /**< RT7xx tar: FRO0. */
#define NXP_FREQME_RT7XX_TAR_FRO2        NXP_FREQME_RT7XX_TAR(7) /**< RT7xx tar: FRO2. */
#define NXP_FREQME_RT7XX_TAR_GPIO_A_CLK  NXP_FREQME_RT7XX_TAR(8) /**< RT7xx tar: GPIO A clock. */
#define NXP_FREQME_RT7XX_TAR_GPIO_B_CLK  NXP_FREQME_RT7XX_TAR(9) /**< RT7xx tar: GPIO B clock. */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MONITOR_NXP_FREQME_H_ */
