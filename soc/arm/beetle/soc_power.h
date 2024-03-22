/*
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_POWER_H_
#define _SOC_POWER_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Setup various clocks and wakeup sources in the SoC.
 *
 * Configures the clocks and wakeup sources in the SoC.
 */
void soc_power_init(void);

#ifdef __cplusplus
}
#endif

#endif /* _SOC_POWER_H_ */
