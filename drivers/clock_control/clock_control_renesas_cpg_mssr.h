/*
 * Copyright (c) 2022 IoT.bzh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_RENESAS_RENESAS_CPG_MSSR_H_
#define ZEPHYR_DRIVERS_RENESAS_RENESAS_CPG_MSSR_H_

#ifdef CONFIG_SOC_SERIES_RCAR_GEN3
/* Software Reset Clearing Register offsets */
#define SRSTCLR(i)      (0x940 + (i) * 4)

/* CPG write protect offset */
#define CPGWPR          0x900

/* Realtime Module Stop Control Register offsets */
static const uint16_t mstpcr[] = {
	0x110, 0x114, 0x118, 0x11c,
	0x120, 0x124, 0x128, 0x12c,
	0x980, 0x984, 0x988, 0x98c,
};

/* Software Reset Register offsets */
static const uint16_t srcr[] = {
	0x0A0, 0x0A8, 0x0B0, 0x0B8,
	0x0BC, 0x0C4, 0x1C8, 0x1CC,
	0x920, 0x924, 0x928, 0x92C,
};

/* CAN-FD Clock Frequency Control Register */
#define CANFDCKCR                 0x244

/* Clock stop bit */
#define CANFDCKCR_CKSTP           BIT(8)

/* CANFD Clock */
#define CANFDCKCR_PARENT_CLK_RATE 800000000
#define CANFDCKCR_DIVIDER_MASK    0x1FF

/* Peripherals Clocks */
#define S3D4_CLK_RATE             66600000	/* SCIF	*/
#define S0D12_CLK_RATE            66600000	/* PWM	*/
#endif /* CONFIG_SOC_SERIES_RCAR_GEN3 */

void rcar_cpg_write(uint32_t base_address, uint32_t reg, uint32_t val);

int rcar_cpg_mstp_clock_endisable(uint32_t base_address, uint32_t module, bool enable);

#endif /* ZEPHYR_DRIVERS_RENESAS_RENESAS_CPG_MSSR_H_ */
