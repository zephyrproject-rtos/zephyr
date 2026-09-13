/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_CLOCK_H_
#define _SOC_CLOCK_H_

/*!
 * @brief Bring the SoC clock tree up to the Over Drive Run operating point.
 *
 * Programs the analog sources and the clock roots described in devicetree, via
 * the power-mode transition that has to bracket a frequency change. Call once,
 * early, before any driver: the peripheral clock roots that live in devicetree
 * as nxp,imx-ccm-rev3-root nodes are applied afterwards by the clock controller
 * and assume the sources this brings up.
 */
void soc_clock_init(void);

#endif /* _SOC_CLOCK_H_ */
