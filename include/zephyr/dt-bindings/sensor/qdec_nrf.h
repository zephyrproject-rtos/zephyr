/*
 * Copyright 2025 Mark Geiger <MarkGeiger@posteo.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the Nordic nRF quadrature decoder.
 * @ingroup qdec_nrf_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_QDEC_NRF_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_QDEC_NRF_H_

/**
 * @defgroup qdec_nrf_interface QDEC nRF
 * @ingroup sensor_interface_ext_nordic
 * @brief Nordic nRF quadrature decoder (QDEC)
 * @{
 */

/**
 * @name Sample period options
 *
 * Values for the `nordic,period` devicetree property.
 * @{
 */
#define SAMPLEPER_128US         0 /**< 128 µs */
#define SAMPLEPER_256US         1 /**< 256 µs */
#define SAMPLEPER_512US         2 /**< 512 µs */
#define SAMPLEPER_1024US        3 /**< 1024 µs */
#define SAMPLEPER_2048US        4 /**< 2048 µs */
#define SAMPLEPER_4096US        5 /**< 4096 µs */
#define SAMPLEPER_8192US        6 /**< 8192 µs */
#define SAMPLEPER_16384US       7 /**< 16384 µs */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_QDEC_NRF_H_ */
