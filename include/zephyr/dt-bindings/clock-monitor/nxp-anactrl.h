/*
 * SPDX-FileCopyrightText: 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MONITOR_NXP_ANACTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MONITOR_NXP_ANACTRL_H_

/**
 * @file
 * @brief INPUTMUX connection cookies for the NXP ANACTRL frequency-measure
 *        reference / target routing.
 *
 * Each cookie is an inputmux_connection_t value matching the device's MCUX HAL
 * header fsl_inputmux_connections.h:
 *
 *   cookie = selector + (destination_register << PMUX_SHIFT)
 *
 * The device tree and driver simply attach the cookie; the hardware routes it
 * to the encoded REF / TARGET register. Selector sets are
 * SoC-series specific, so they carry a series infix (e.g. MCXW2XX).
 */

/** Bit position of the destination mux register within a connection cookie. */
#define NXP_ANACTRL_PMUX_SHIFT 20U

/**
 * @brief set_source() sentinel: leave that axis (reference or target) unchanged.
 *
 * A real INPUTMUX connection cookie is always nonzero (it always carries a
 * destination-register field in its upper bits), so 0 is free to mean
 * "no change" for clock_monitor_set_source().
 */
#define NXP_ANACTRL_SOURCE_UNCHANGED 0U

/*
 * MCXW2xx selectors.
 * ANACTRL reference / target mux registers at 0x180 / 0x184.
 */

/**  reference mux register offset (MCXW2xx). */
#define NXP_ANACTRL_MCXW2XX_REF_REG 0x180U
/** ANACTRL target mux register offset (MCXW2xx). */
#define NXP_ANACTRL_MCXW2XX_TAR_REG 0x184U

/** Build an MCXW2xx reference-route cookie from selector @p sel. */
#define NXP_ANACTRL_MCXW2XX_REF(sel)                                                               \
	((sel) + (NXP_ANACTRL_MCXW2XX_REF_REG << NXP_ANACTRL_PMUX_SHIFT))
/** Build an MCXW2xx target-route cookie from selector @p sel. */
#define NXP_ANACTRL_MCXW2XX_TAR(sel)                                                               \
	((sel) + (NXP_ANACTRL_MCXW2XX_TAR_REG << NXP_ANACTRL_PMUX_SHIFT))

/* MCXW2xx reference sources. */
/** external OSC (32 MHz crystal). */
#define NXP_ANACTRL_MCXW2XX_REF_EXTERN_OSC  NXP_ANACTRL_MCXW2XX_REF(0)
/** 12 MHz FRO. */
#define NXP_ANACTRL_MCXW2XX_REF_FRO_12M     NXP_ANACTRL_MCXW2XX_REF(1)
/** 32 MHz FRO. */
#define NXP_ANACTRL_MCXW2XX_REF_FRO_32M     NXP_ANACTRL_MCXW2XX_REF(2)
/** 1 MHz FRO. */
#define NXP_ANACTRL_MCXW2XX_REF_FRO_1M      NXP_ANACTRL_MCXW2XX_REF(3)
/** 32 kHz RTC oscillator. */
#define NXP_ANACTRL_MCXW2XX_REF_OSC32K      NXP_ANACTRL_MCXW2XX_REF(4)
/** main / system AHB clock. */
#define NXP_ANACTRL_MCXW2XX_REF_MAIN_CLK    NXP_ANACTRL_MCXW2XX_REF(5)
/** SPIFI clock. */
#define NXP_ANACTRL_MCXW2XX_REF_SPIFI_CLK   NXP_ANACTRL_MCXW2XX_REF(6)
/** PORT0 pin 3 input. */
#define NXP_ANACTRL_MCXW2XX_REF_PORT0_PIN3  NXP_ANACTRL_MCXW2XX_REF(7)
/** PORT0 pin 11 input. */
#define NXP_ANACTRL_MCXW2XX_REF_PORT0_PIN11 NXP_ANACTRL_MCXW2XX_REF(8)
/** PORT0 pin 12 input. */
#define NXP_ANACTRL_MCXW2XX_REF_PORT0_PIN12 NXP_ANACTRL_MCXW2XX_REF(9)
/** PORT0 pin 14 input. */
#define NXP_ANACTRL_MCXW2XX_REF_PORT0_PIN14 NXP_ANACTRL_MCXW2XX_REF(10)

/* MCXW2xx target sources. */
/** external OSC (32 MHz crystal). */
#define NXP_ANACTRL_MCXW2XX_TAR_EXTERN_OSC  NXP_ANACTRL_MCXW2XX_TAR(0)
/** 12 MHz FRO. */
#define NXP_ANACTRL_MCXW2XX_TAR_FRO_12M     NXP_ANACTRL_MCXW2XX_TAR(1)
/** 32 MHz FRO. */
#define NXP_ANACTRL_MCXW2XX_TAR_FRO_32M     NXP_ANACTRL_MCXW2XX_TAR(2)
/** 1 MHz FRO. */
#define NXP_ANACTRL_MCXW2XX_TAR_FRO_1M      NXP_ANACTRL_MCXW2XX_TAR(3)
/** 32 kHz RTC oscillator. */
#define NXP_ANACTRL_MCXW2XX_TAR_OSC32K      NXP_ANACTRL_MCXW2XX_TAR(4)
/** main / system AHB clock. */
#define NXP_ANACTRL_MCXW2XX_TAR_MAIN_CLK    NXP_ANACTRL_MCXW2XX_TAR(5)
/** SPIFI clock. */
#define NXP_ANACTRL_MCXW2XX_TAR_SPIFI_CLK   NXP_ANACTRL_MCXW2XX_TAR(6)
/** PORT0 pin 3 input. */
#define NXP_ANACTRL_MCXW2XX_TAR_PORT0_PIN3  NXP_ANACTRL_MCXW2XX_TAR(7)
/** PORT0 pin 11 input. */
#define NXP_ANACTRL_MCXW2XX_TAR_PORT0_PIN11 NXP_ANACTRL_MCXW2XX_TAR(8)
/** PORT0 pin 12 input. */
#define NXP_ANACTRL_MCXW2XX_TAR_PORT0_PIN12 NXP_ANACTRL_MCXW2XX_TAR(9)
/** PORT0 pin 14 input. */
#define NXP_ANACTRL_MCXW2XX_TAR_PORT0_PIN14 NXP_ANACTRL_MCXW2XX_TAR(10)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MONITOR_NXP_ANACTRL_H_ */
