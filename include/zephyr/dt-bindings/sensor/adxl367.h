/*
 * Copyright (c) 2025 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ADI_ADX367_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ADI_ADX367_H_

/**
 * @defgroup ADXL367 ADI DT Options
 * @ingroup sensor_interface
 * @{
 */

/**
 * @defgroup ADXL367_FIFO_MODE FIFO mode options
 * @{
 */
#define ADXL367_FIFO_MODE_DISABLED        0x0
#define ADXL367_FIFO_MODE_OLDEST_SAVED    0x1
#define ADXL367_FIFO_MODE_STREAM          0x2
#define ADXL367_FIFO_MODE_TRIGGERED       0x3
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ADI_ADX367_H_ */
