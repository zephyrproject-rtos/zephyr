/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_XSPI_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_XSPI_H_

/**
 * @name XSPI definition for the xSPI peripherals
 * Note that
 *  SPI mode impossible in STR transfer rate only
 */

/* XSPI mode operating on 1 line, 2 lines, 4 lines or 8 lines */
/* 1 Cmd Line, 1 Address Line and 1 Data Line    */
#define XSPI_SPI_MODE                     1
/* 2 Cmd Lines, 2 Address Lines and 2 Data Lines */
#define XSPI_DUAL_MODE                    2
/* 4 Cmd Lines, 4 Address Lines and 4 Data Lines */
#define XSPI_QUAD_MODE                    4
/* 8 Cmd Lines, 8 Address Lines and 8 Data Lines */
#define XSPI_OCTO_MODE                    8

/* XSPI mode operating on Single or Double Transfer Rate */
/* Single Transfer Rate */
#define XSPI_STR_TRANSFER                 1
/* Double Transfer Rate */
#define XSPI_DTR_TRANSFER                 2

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_XSPI_H_ */
