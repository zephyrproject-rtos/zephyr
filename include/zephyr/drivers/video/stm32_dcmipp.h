/*
 * Copyright (c) 2025 STMicroelectronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_VIDEO_STM32_DCMIPP_H_
#define ZEPHYR_INCLUDE_VIDEO_STM32_DCMIPP_H_

/* Prototypes of ISP external handler weak functions */
void stm32_dcmipp_isp_vsync_update(DCMIPP_HandleTypeDef *hdcmipp, uint32_t Pipe);
int stm32_dcmipp_isp_init(DCMIPP_HandleTypeDef *hdcmipp, const struct device *source);
int stm32_dcmipp_isp_start(void);
int stm32_dcmipp_isp_stop(void);

#endif /* ZEPHYR_INCLUDE_VIDEO_STM32_DCMIPP_H_ */
