/*
 * Copyright (c) 2025 STMicroelectronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ISP handler hooks for the STM32 DCMIPP video driver.
 * @ingroup video_interface
 */

#ifndef ZEPHYR_INCLUDE_VIDEO_STM32_DCMIPP_H_
#define ZEPHYR_INCLUDE_VIDEO_STM32_DCMIPP_H_

/* Prototypes of ISP external handler weak functions */

/**
 * @brief Notify the external ISP handler of a VSYNC event
 *
 * The DCMIPP driver calls this hook on each VSYNC event, indicating that new
 * statistics are available. A weak no-op implementation is provided; an
 * external ISP handler can override it.
 *
 * @param hdcmipp HAL DCMIPP handle
 * @param Pipe DCMIPP pipe on which the VSYNC event occurred
 */
void stm32_dcmipp_isp_vsync_update(DCMIPP_HandleTypeDef *hdcmipp, uint32_t Pipe);

/**
 * @brief Initialize the external ISP handler
 *
 * The DCMIPP driver calls this hook during driver initialization. A weak
 * no-op implementation is provided; an external ISP handler can override it.
 *
 * @param hdcmipp HAL DCMIPP handle
 * @param source Pointer to the video source device
 *
 * @return 0 on success, negative errno code on failure
 */
int stm32_dcmipp_isp_init(DCMIPP_HandleTypeDef *hdcmipp, const struct device *source);

/**
 * @brief Start the external ISP handler
 *
 * The DCMIPP driver calls this hook when video capture starts. A weak no-op
 * implementation is provided; an external ISP handler can override it.
 *
 * @return 0 on success, negative errno code on failure
 */
int stm32_dcmipp_isp_start(void);

/**
 * @brief Stop the external ISP handler
 *
 * The DCMIPP driver calls this hook when video capture stops. A weak no-op
 * implementation is provided; an external ISP handler can override it.
 *
 * @return 0 on success, negative errno code on failure
 */
int stm32_dcmipp_isp_stop(void);

#endif /* ZEPHYR_INCLUDE_VIDEO_STM32_DCMIPP_H_ */
