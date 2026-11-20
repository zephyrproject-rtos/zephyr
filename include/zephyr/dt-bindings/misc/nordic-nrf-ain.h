/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2026 Nordic Semiconductor ASA
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_MISC_NORDIC_NRF_AIN_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_MISC_NORDIC_NRF_AIN_H_

/**
 * @brief Cells of the ain-gpios property of the nordic,nrf-ain node.
 *
 * Expands NRFX_AIN_TABLE, provided by the SoC specific nrfx_analog/ header, into
 * a list of cells with one GPIO specifier per AIN and 0 for each AIN that does
 * not exist. Use it as: ain-gpios = <NRF_AIN_DT_GPIOS>;
 */
#define NRF_AIN_DT_GPIOS NRFX_AIN_TABLE(NRF_AIN_DT_ENTRY, NRF_AIN_DT_NA)

#define NRF_AIN_DT_ENTRY(idx, port, pin, saadc, comp, lpcomp) &gpio##port pin 0
#define NRF_AIN_DT_NA(idx) 0

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_MISC_NORDIC_NRF_AIN_H_ */
