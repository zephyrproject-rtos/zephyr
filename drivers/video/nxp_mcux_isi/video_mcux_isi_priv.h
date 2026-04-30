/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_VIDEO_NXP_MCUX_ISI_PRIV_H_
#define ZEPHYR_DRIVERS_VIDEO_NXP_MCUX_ISI_PRIV_H_

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>

#include <fsl_isi.h>

struct video_mcux_isi_core_config {
	ISI_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
};

struct video_mcux_isi_core_data {
	ISI_Type *base;
};

const struct video_mcux_isi_core_data *video_mcux_isi_core_get_data(const struct device *core_dev);

#endif /* ZEPHYR_DRIVERS_VIDEO_NXP_MCUX_ISI_PRIV_H_ */
