/*
 * Copyright 2023 Chirp Microsystems. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file chirp_board_config.h
 *
 * This file defines required symbols used to build an application with the Chirp SonicLib
 * API and driver.  These symbols are used for static array allocations and counters in SonicLib
 * (and often applications), and are based on the number of specific resources on the target board.
 *
 * Two symbols must be defined:
 *  CHIRP_MAX_NUM_SENSORS - the number of possible sensor devices (i.e. the number of sensor ports)
 *  CHIRP_NUM_I2C_BUSES - the number of I2C buses on the board that are used for those sensor ports
 *
 * This file must be in the C pre-processor include path when the application is built with SonicLib
 * and this board support package.
 */

#ifndef CHIRP_BOARD_CONFIG_H
#define CHIRP_BOARD_CONFIG_H

/* Settings for the Chirp SmartSonic board */
/** maximum possible number of sensor devices */
#define CHIRP_MAX_NUM_SENSORS 1
/** number of I2C buses used by sensors */
#define CHIRP_NUM_I2C_BUSES   1

/* Deactivate use of debug I2C interface */
#define USE_STD_I2C_FOR_IQ (1)

#endif /* CHIRP_BOARD_CONFIG_H */
