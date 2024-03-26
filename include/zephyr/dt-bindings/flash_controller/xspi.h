/*
 * Copyright (c) 2022 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_XSPI_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_XSPI_H_

/**
 * @name XSPI definition for the OctoSPI peripherals
 * Note that the possible combination is
 *  SPI mode in STR transfer rate
 *  OPI mode in STR transfer rate
 *  OPI mode in DTR transfer rate
 */

/* XSPI mode operating on 1 line, 2 lines, 4 lines or 8 lines */
/* 1 Cmd Line, 1 Address Line and 1 Data Line    */
#define XSPI_SPI_MODE                     1
#define OSPI_SPI_MODE                     XSPI_SPI_MODE
/* 2 Cmd Lines, 2 Address Lines and 2 Data Lines */
#define XSPI_DUAL_MODE                    2
#define OSPI_DUAL_MODE                    XSPI_DUAL_MODE
/* 4 Cmd Lines, 4 Address Lines and 4 Data Lines */
#define XSPI_QUAD_MODE                    4
#define OSPI_QUAD_MODE                    XSPI_QUAD_MODE
/* 8 Cmd Lines, 8 Address Lines and 8 Data Lines */
#define XSPI_OCTO_MODE                    8
#define OSPI_OCTO_MODE                    XSPI_OCTO_MODE

/* XSPI mode operating on Single or Double Transfer Rate */
/* Single Transfer Rate */
#define XSPI_STR_TRANSFER                 1
#define OSPI_STR_TRANSFER                 XSPI_STR_TRANSFER
/* Double Transfer Rate */
#define XSPI_DTR_TRANSFER                 2
#define OSPI_DTR_TRANSFER                 XSPI_DTR_TRANSFER

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_XSPI_H_ */
