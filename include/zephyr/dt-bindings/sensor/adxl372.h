/*
 * Copyright (c) 2025 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ADI_ADX372_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ADI_ADX372_H_

/**
 * @defgroup ADXL372 ADI DT Options
 * @ingroup sensor_interface
 * @{
 */

/**
 * @defgroup ADXL372_FIFO_MODE FIFO mode options
 * @{
 */

#define ADXL372_FIFO_MODE_BYPASSED        0x0
#define ADXL372_FIFO_MODE_STREAMED        0x1
#define ADXL372_FIFO_MODE_TRIGGERED       0x2
#define ADXL372_FIFO_MODE_OLD_SAVED       0x3

/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ADI_ADX372_H_ */
