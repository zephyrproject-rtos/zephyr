/*
 * Copyright (c) 2022 IoT.bzh
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_SOC_ARM_RENESAS_RCAR_GEN3_CLOCK_SOC_H_
#define ZEPHYR_SOC_ARM_RENESAS_RCAR_GEN3_CLOCK_SOC_H_

#include <clock_rcar.h>

/* Software Reset Clearing Register offsets */
#define SRSTCLR(i)      (0x940 + (i) * 4)
/* CPG write protect offset */
#define CPGWPR          0x0900

#endif /* ZEPHYR_SOC_ARM_RENESAS_RCAR_GEN3_CLOCK_SOC_H_ */
