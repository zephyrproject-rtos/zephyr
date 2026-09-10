/*
 * Copyright (c) 2017, 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_KINETIS_SIM_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_KINETIS_SIM_H_

#define KINETIS_SIM_CORESYS_CLK		0
#define KINETIS_SIM_PLATFORM_CLK	1
#define KINETIS_SIM_BUS_CLK		2
#define KINETIS_SIM_FAST_PERIPHERAL_CLK	5
#define KINETIS_SIM_LPO_CLK		19
#define KINETIS_SIM_DMAMUX_CLK  KINETIS_SIM_BUS_CLK
#define KINETIS_SIM_DMA_CLK     KINETIS_SIM_CORESYS_CLK
#define KINETIS_SIM_SIM_SOPT7   7
#define KINETIS_SIM_OSCERCLK    8
#define KINETIS_SIM_MCGIRCLK    12
#define KINETIS_SIM_MCGPCLK     18

#define KINETIS_SIM_PLLFLLSEL_MCGFLLCLK	0
#define KINETIS_SIM_PLLFLLSEL_MCGPLLCLK	1
#define KINETIS_SIM_PLLFLLSEL_IRC48MHZ	3

#define KINETIS_SIM_ER32KSEL_OSC32KCLK	0
#define KINETIS_SIM_ER32KSEL_RTC	2
#define KINETIS_SIM_ER32KSEL_LPO1KHZ	3

#define KINETIS_SIM_ENET_CLK		4321
#define KINETIS_SIM_ENET_1588_CLK	4322

#define KINETIS_SIM_CMP_CLK		4323
#define KINETIS_SIM_VREF_CLK		4324

/*
 * SIM clock specifier encoding.
 *
 * Each SIM clock reference is a 3-cell specifier \<id offset bits\>:
 *
 *   id     - 32-bit encoded value containing gate and rate information
 *   offset - gate register offset (explicit, for readability and
 *             consumers such as ADC16 that read it directly)
 *   bits   - gate bit index or mask (explicit, same reason)
 *
 * Layout of the `id` cell (32-bit):
 *   [31:19] gate_offset (13 bits) - SCGC register offset; 0 = no gate
 *   [18:14] gate_bit    (5 bits)  - bit position within SCGC register
 *   [13:9]  clock_name  (5 bits)  - KINETIS_SIM_*_CLK for get_rate
 *   [8:6]   mux_val     (3 bits)  - mux source value for configure
 *   [5:1]   mux_shift   (5 bits)  - mux bit shift in mux register
 *   [0]     has_mux     (1 bit)   - 1 = configure mux on .configure call
 *
 * All known clock name values (0-19) and gate offsets (up to 0x1040)
 * fit within their respective fields.
 */

/** @cond INTERNAL_HIDDEN */
#define KINETIS_SIM_CLOCK_ID(name, gate_offset, gate_bit) \
	((((gate_offset) & 0x1FFFU) << 19) | \
	 (((gate_bit)    & 0x1FU)   << 14) | \
	 (((name)        & 0x1FU)   <<  9))

#define KINETIS_SIM_CLOCK_ID_MUX(name, gate_offset, gate_bit, mux_shift, mux_val) \
	(KINETIS_SIM_CLOCK_ID(name, gate_offset, gate_bit) | \
	 (((mux_val)   & 0x7U) << 6) | \
	 (((mux_shift) & 0x1FU) << 1) | 0x1U)
/** @endcond */

/** @cond INTERNAL_HIDDEN */
/* Decoders used by the driver and soc.c */
#define KINETIS_SIM_CLOCK_DECODE_GATE_OFFSET(val)  (((val) >> 19) & 0x1FFFU)
#define KINETIS_SIM_CLOCK_DECODE_GATE_BIT(val)     (((val) >> 14) & 0x1FU)
#define KINETIS_SIM_CLOCK_DECODE_NAME(val)         (((val) >>  9) & 0x1FU)
#define KINETIS_SIM_CLOCK_DECODE_MUX_VAL(val)      (((val) >>  6) & 0x7U)
#define KINETIS_SIM_CLOCK_DECODE_MUX_SHIFT(val)    (((val) >>  1) & 0x1FU)
#define KINETIS_SIM_CLOCK_HAS_MUX(val)             ((val) & 0x1U)
/** @endcond */

/**
 * @def KINETIS_SIM_CLOCK
 * @brief Build a 3-cell SIM clock specifier.
 *
 * Expands to \<id offset bits\> where `id` encodes gate and rate information,
 * and `offset`/`bits` are kept explicit for readability and for consumers
 * (e.g. ADC16) that read them as separate cells.
 *
 * @param name   Clock name selector (KINETIS_SIM_*_CLK)
 * @param offset Gate register offset (e.g. 0x1034)
 * @param bits   Gate bit index (0..31)
 */
#define KINETIS_SIM_CLOCK(name, offset, bits) \
	KINETIS_SIM_CLOCK_ID(name, offset, bits) offset bits

/**
 * @def KINETIS_SIM_CLOCK_MUX
 * @brief Build a 3-cell SIM clock specifier with mux configuration.
 *
 * Like KINETIS_SIM_CLOCK but also encodes the mux register field and value
 * needed for clock_control_configure() support.
 *
 * @param name      Clock name selector (KINETIS_SIM_*_CLK)
 * @param offset    Gate register offset
 * @param bits      Gate bit index (0..31)
 * @param mux_shift Bit shift of the mux field in the mux register (e.g. SOPT2)
 * @param mux_val   Value to write into the mux field
 */
#define KINETIS_SIM_CLOCK_MUX(name, offset, bits, mux_shift, mux_val) \
	KINETIS_SIM_CLOCK_ID_MUX(name, offset, bits, mux_shift, mux_val) offset bits

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_KINETIS_SIM_H_ */
