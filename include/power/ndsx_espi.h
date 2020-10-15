/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __NDSX_ESPI_H__
#define __NDSX_ESPI_H__

/**
 * @brief Configure eSPI channels for Non Deep Slwwp well platforms
 */
void ndsx_espi_configure(void);

/**
 * @brief Return Virtual wire value

 * @retval Acatual Virtual Wire value or 0 on failure
 */
uint8_t vw_get_level(enum espi_vwire_signal signal);

#endif /* __NDSX_ESPI_H__ */
