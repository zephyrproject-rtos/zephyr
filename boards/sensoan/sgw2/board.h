/*
 * Copyright (c) 2024 Sensoan Oy
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BOARD_H
#define BOARD_H

/* Internal macros */
#define PORT(_TYPE, _PIN)   _TYPE##_PORT_##_PIN
#define PIN(_TYPE, _PIN)    _TYPE##_PIN_##_PIN
#define GPIOAUX(_PORT)      &gpio##_PORT
#define GPIO_BY_PORT(_PORT) GPIOAUX(_PORT)
#define GPIO(_TYPE, _PIN)   GPIO_BY_PORT(PORT(_TYPE, _PIN))

/**
 * Rewrite a pin with a special type as the corresponding GPIO node and pin.
 * These correspondences are encoded in the header files sgw2_nrf5340.h and
 * sgw2_nrf9160.h contained in the board directory.
 *
 * @param _TYPE Pin type (allowed types are MCU_IF and EDGE_CONN)
 * @param _PIN Pin number
 */
#define GPIO_AND_PIN(_TYPE, _PIN) GPIO(_TYPE, _PIN) PIN(_TYPE, _PIN)

/**
 * A wrapper for the NRF_PSEL macro that accepts pins with special types.
 * These are translated into the corresponding GPIO ports and pins
 * for the use of NRF_PSEL via the correspondences encoded in the header files
 * sgw2_nrf5340.h and sgw2_nrf9160.h contained in the board directory.
 *
 * @param _FUN Pin function configuration passed to the NRF_PSEL macro
 * @param _TYPE Pin type (allowed types are MCU_IF and EDGE_CONN)
 * @param _PIN Pin number
 */
#define SGW_PSEL(_FUN, _TYPE, _PIN) NRF_PSEL(_FUN, PORT(_TYPE, _PIN), PIN(_TYPE, _PIN))

#endif /* BOARD_H */
