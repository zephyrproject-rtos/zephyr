/*
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __FLASH_MSPI_NOR_MICRON_H__
#define __FLASH_MSPI_NOR_MICRON_H__

#include "flash_mspi_nor.h"

extern const struct flash_mspi_nor_vendor micron_vendor;

/* MX25R support macros */
#define FLASH_mxicy_mx25r_MSPI_IO_MODE_SINGLE_SUPPORTED 1
#define FLASH_mxicy_mx25r_MSPI_IO_MODE_SINGLE_DATA mxicy_mx25r_single
extern struct flash_mspi_mode_data mxicy_mx25r_single;
#define FLASH_mxicy_mx25r_MSPI_IO_MODE_QUAD_1_4_4_SUPPORTED 1
#define FLASH_mxicy_mx25r_MSPI_IO_MODE_QUAD_1_4_4_DATA mxicy_mx25r_quad
extern struct flash_mspi_mode_data mxicy_mx25r_quad;

/* MX25U support macros */
#define FLASH_mxicy_mx25u_MSPI_IO_MODE_SINGLE_SUPPORTED 1
#define FLASH_mxicy_mx25u_MSPI_IO_MODE_SINGLE_DATA mxicy_mx25u_single
extern struct flash_mspi_mode_data mxicy_mx25u_single;
#define FLASH_mxicy_mx25u_MSPI_IO_MODE_OCTAL_SUPPORTED 1
#define FLASH_mxicy_mx25u_MSPI_IO_MODE_OCTAL_DATA mxicy_mx25u_octal
extern struct flash_mspi_mode_data mxicy_mx25u_octal;

/* MT35 support macros */
#define FLASH_mt35xu02gcba_MSPI_IO_MODE_OCTAL_1_8_8_SUPPORTED 1
#define FLASH_mt35xu02gcba_MSPI_IO_MODE_OCTAL_1_8_8_DATA mt35xu02gcba_octal
extern struct flash_mspi_mode_data mt35xu02gcba_octal;

/* MT25 support macros */
#define FLASH_mt25qu512abb_MSPI_IO_MODE_OCTAL_1_4_4_SUPPORTED 1
#define FLASH_mt25qu512abb_MSPI_IO_MODE_OCTAL_1_4_4_DATA mt25qu512abb_quad
extern struct flash_mspi_mode_data mt25qu512abb_quad;

#endif /* __FLASH_MSPI_NOR_MICRON_H__ */
