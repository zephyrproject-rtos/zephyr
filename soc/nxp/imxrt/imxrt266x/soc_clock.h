/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_CLOCK_H_
#define _SOC_CLOCK_H_

/**
 * @brief Bring the SoC clock tree up to the Over Drive Run operating point.
 *
 * Programs the analog sources through the power-mode transition a frequency
 * change has to be bracketed by. Call once before any driver: the per-peripheral
 * roots in devicetree are applied later by the clock controller and assume these
 * sources.
 */
void soc_clock_init(void);

#endif /* _SOC_CLOCK_H_ */
