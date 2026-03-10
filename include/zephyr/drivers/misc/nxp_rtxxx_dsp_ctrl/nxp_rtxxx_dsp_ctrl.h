/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/dt-bindings/misc/nxp_rtxxx_dsp_ctrl.h>

#ifndef __NXP_RTXXX_DSP_CTRL_H__
#define __NXP_RTXXX_DSP_CTRL_H__

/**
 * @brief Describes an image section type selection.
 */
enum nxp_rtxxx_dsp_ctrl_section_type {
	NXP_RTXXX_DSP_CTRL_SECTION_RESET = NXP_RTXXX_DSP_REGION_RESET,
	NXP_RTXXX_DSP_CTRL_SECTION_TEXT = NXP_RTXXX_DSP_REGION_TEXT,
	NXP_RTXXX_DSP_CTRL_SECTION_DATA = NXP_RTXXX_DSP_REGION_DATA
};

typedef int (*nxp_rtxxx_dsp_ctrl_api_load_section)(
	const struct device *,
	const void *,
	size_t,
	enum nxp_rtxxx_dsp_ctrl_section_type
);
typedef void (*nxp_rtxxx_dsp_ctrl_api_enable)(const struct device *dev);
typedef void (*nxp_rtxxx_dsp_ctrl_api_disable)(const struct device *dev);

struct nxp_rtxxx_dsp_ctrl_api {
	nxp_rtxxx_dsp_ctrl_api_load_section load_section;
	nxp_rtxxx_dsp_ctrl_api_enable enable;
	nxp_rtxxx_dsp_ctrl_api_disable disable;
};

/**
 * @brief Loads a specified image representing a specified section to a particular region in the
 * DSP's memory.
 *
 * @param dev DSP device
 * @param base Base pointer of the image to load
 * @param length Length of the image
 * @param section Section type which specified image represents
 * @return int 0 on success, -EINVAL for invalid parameters, -ENOMEM for image bigger than the
 * target region
 */
static inline int nxp_rtxxx_dsp_ctrl_load_section(
	const struct device *dev,
	const void *base,
	size_t length,
	enum nxp_rtxxx_dsp_ctrl_section_type section
)
{
	return ((struct nxp_rtxxx_dsp_ctrl_api *)dev->api)
		->load_section(dev, base, length, section);
}

/**
 * @brief Starts (unstalls) the DSP.
 *
 * @param dev DSP device
 */
static inline void nxp_rtxxx_dsp_ctrl_enable(const struct device *dev)
{
	((struct nxp_rtxxx_dsp_ctrl_api *)dev->api)->enable(dev);
}

/**
 * @brief Stops (stalls) the DSP.
 *
 * @param dev DSP device
 */
static inline void nxp_rtxxx_dsp_ctrl_disable(const struct device *dev)
{
	((struct nxp_rtxxx_dsp_ctrl_api *)dev->api)->disable(dev);
}

#endif
