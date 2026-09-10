/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_MLX90640_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_MLX90640_H_

/**
 * @file
 * @brief Devicetree constants for the MLX90640 IR array
 * @ingroup mlx90640_interface
 */

/**
 * @addtogroup mlx90640_interface
 * @{
 */

/** @brief Control register 1 refresh-rate field encodings (bits 9:7) */
#define MLX90640_REFRESH_0_5_HZ 0
#define MLX90640_REFRESH_1_HZ   1
#define MLX90640_REFRESH_2_HZ   2
#define MLX90640_REFRESH_4_HZ   3
#define MLX90640_REFRESH_8_HZ   4
#define MLX90640_REFRESH_16_HZ  5
#define MLX90640_REFRESH_32_HZ  6
#define MLX90640_REFRESH_64_HZ  7

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_MLX90640_H_ */
