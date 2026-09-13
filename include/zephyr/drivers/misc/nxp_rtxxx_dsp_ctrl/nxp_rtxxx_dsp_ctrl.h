/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MISC_NXP_RTXXX_DSP_CTRL_NXP_RTXXX_DSP_CTRL_H_
#define ZEPHYR_INCLUDE_DRIVERS_MISC_NXP_RTXXX_DSP_CTRL_NXP_RTXXX_DSP_CTRL_H_

#include <zephyr/device.h>
#include <zephyr/sys/util.h>

/**
 * @brief Describes a single DSP image segment to be copied from its load
 * address (LMA) to its run address (VMA).
 */
struct nxp_rtxxx_dsp_ctrl_segment {
	/** Load address - where the segment currently resides (e.g. flash). */
	uintptr_t lma;
	/** Run address - where the segment must be copied to in the DSP memory view. */
	uintptr_t vma;
	/** Segment size in bytes. */
	size_t size;
};

__subsystem struct nxp_rtxxx_dsp_ctrl_api {
	/**
	 * @driver_ops_mandatory @copybrief nxp_rtxxx_dsp_ctrl_load
	 * See nxp_rtxxx_dsp_ctrl_load for arguments description.
	 */
	int (*load)(
		const struct device *dev,
		const struct nxp_rtxxx_dsp_ctrl_segment *segments,
		size_t count
	);

	/**
	 * @driver_ops_mandatory @copybrief nxp_rtxxx_dsp_ctrl_enable
	 * See nxp_rtxxx_dsp_ctrl_enable for arguments description.
	 */
	void (*enable)(const struct device *dev);

	/**
	 * @driver_ops_mandatory @copybrief nxp_rtxxx_dsp_ctrl_disable
	 * See nxp_rtxxx_dsp_ctrl_disable for arguments description.
	 */
	void (*disable)(const struct device *dev);
};

/**
 * @brief Loads the DSP image by copying each of the given segments from its
 * load address (LMA) to its run address (VMA) in the DSP's memory view.
 *
 * @param dev DSP device
 * @param segments Array of segment descriptors (LMA, VMA, size)
 * @param count Number of segments in the array
 * @return int 0 on success, -EINVAL for invalid parameters
 */
static inline int nxp_rtxxx_dsp_ctrl_load(
	const struct device *dev,
	const struct nxp_rtxxx_dsp_ctrl_segment *segments,
	size_t count
)
{
	return ((struct nxp_rtxxx_dsp_ctrl_api *)dev->api)->load(dev, segments, count);
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

/**
 * @brief Builds a single @ref nxp_rtxxx_dsp_ctrl_segment initializer for
 * segment @p n from the generated image information header.
 *
 * Intended for use with the image information header (zephyr_image_info.h)
 * produced for the remote (DSP) image by gen_image_info.py when
 * CONFIG_BUILD_OUTPUT_INFO_HEADER is set. That header must be included by the
 * translation unit that expands this macro.
 */
#define NXP_RTXXX_DSP_CTRL_SEGMENT(n, _) \
	{ \
		.lma = SEGMENT_LMA_ADDRESS_##n, \
		.vma = SEGMENT_VMA_ADDRESS_##n, \
		.size = SEGMENT_SIZE_##n, \
	}

/**
 * @brief Builds an array initializer of @ref nxp_rtxxx_dsp_ctrl_segment entries
 * for every segment described by the generated image information header.
 *
 * Expands to a brace-enclosed list built from SEGMENT_NUM and the token-paste
 * macros SEGMENT_LMA_ADDRESS_<n> / SEGMENT_VMA_ADDRESS_<n> / SEGMENT_SIZE_<n>
 * exposed by zephyr_image_info.h, which must be included by the translation
 * unit that expands this macro. Use it to initialize an array, e.g.:
 *
 * @code
 * static const struct nxp_rtxxx_dsp_ctrl_segment segments[] =
 *	NXP_RTXXX_DSP_CTRL_SEGMENTS_FROM_IMAGE_INFO();
 * @endcode
 */
#define NXP_RTXXX_DSP_CTRL_SEGMENTS_FROM_IMAGE_INFO() \
	{ \
		LISTIFY(SEGMENT_NUM, NXP_RTXXX_DSP_CTRL_SEGMENT, (,)) \
	}

#endif /* ZEPHYR_INCLUDE_DRIVERS_MISC_NXP_RTXXX_DSP_CTRL_NXP_RTXXX_DSP_CTRL_H_ */
