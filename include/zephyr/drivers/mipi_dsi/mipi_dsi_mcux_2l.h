/*
 * Copyright 2023,2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_MIPI_DSI_MCUX_2L_
#define ZEPHYR_INCLUDE_DRIVERS_MIPI_DSI_MCUX_2L_

/*
 * HW specific flag- indicates to the MIPI DSI 2L peripheral that the
 * data being sent is framebuffer data, which the DSI peripheral may
 * byte swap depending on KConfig settings
 */
#define MCUX_DSI_2L_FB_DATA BIT(0x1)

/*
 * HW specific flag - user set this bit in message flag and after the
 * transfer bus will enter ULPS.
 */
#define MCUX_DSI_2L_ULPS BIT(0x2)

#endif /* ZEPHYR_INCLUDE_DRIVERS_MIPI_DSI_MCUX_2L_ */
