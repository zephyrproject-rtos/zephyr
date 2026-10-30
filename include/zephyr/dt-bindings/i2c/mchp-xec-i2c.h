/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_I2C_MCHP_XEC_I2C_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_I2C_MCHP_XEC_I2C_H_

/**
 * @file
 * @brief Devicetree binding constants for the Microchip XEC I2C controller.
 */

/** @brief Encode a controller index and port index into a single cell. */
#define MCHP_XEC_I2C_CTRL_PORT(ctrl, port) (((ctrl & 0xf) << 4) | (port & 0xf))

/** @brief I2C port 0. */
#define MCHP_XEC_I2C_PORT0    0
/** @brief I2C port 1. */
#define MCHP_XEC_I2C_PORT1    1
/** @brief I2C port 2. */
#define MCHP_XEC_I2C_PORT2    2
/** @brief I2C port 3. */
#define MCHP_XEC_I2C_PORT3    3
/** @brief I2C port 4. */
#define MCHP_XEC_I2C_PORT4    4
/** @brief I2C port 5. */
#define MCHP_XEC_I2C_PORT5    5
/** @brief I2C port 6. */
#define MCHP_XEC_I2C_PORT6    6
/** @brief I2C port 7. */
#define MCHP_XEC_I2C_PORT7    7
/** @brief I2C port 8. */
#define MCHP_XEC_I2C_PORT8    8
/** @brief I2C port 9. */
#define MCHP_XEC_I2C_PORT9    9
/** @brief I2C port 10. */
#define MCHP_XEC_I2C_PORT10   10
/** @brief I2C port 11. */
#define MCHP_XEC_I2C_PORT11   11
/** @brief I2C port 12. */
#define MCHP_XEC_I2C_PORT12   12
/** @brief I2C port 13. */
#define MCHP_XEC_I2C_PORT13   13
/** @brief I2C port 14. */
#define MCHP_XEC_I2C_PORT14   14
/** @brief I2C port 15. */
#define MCHP_XEC_I2C_PORT15   15
/** @brief Number of I2C ports supported. */
#define MCHP_XEC_I2C_PORT_MAX 16

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_I2C_MCHP_XEC_I2C_H_ */
