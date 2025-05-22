/*
 * Copyright (c) 2022 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_OSPI_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_OSPI_H_

/**
 * @name OSPI definition for the OctoSPI peripherals
 * Note that the possible combination is
 *  SPI mode in STR transfer rate
 *  OPI mode in STR transfer rate
 *  OPI mode in DTR transfer rate
 */

/* OSPI mode operating on 1 line, 2 lines, 4 lines or 8 lines */
/* 1 Cmd Line, 1 Address Line and 1 Data Line    */
#define OSPI_SPI_MODE                     1
/* 2 Cmd Lines, 2 Address Lines and 2 Data Lines */
#define OSPI_DUAL_MODE                    2
/* 4 Cmd Lines, 4 Address Lines and 4 Data Lines */
#define OSPI_QUAD_MODE                    4
/* 8 Cmd Lines, 8 Address Lines and 8 Data Lines */
#define OSPI_OPI_MODE                     8

/* OSPI mode operating on Single or Double Transfer Rate */
/* Single Transfer Rate */
#define OSPI_STR_TRANSFER                 1
/* Double Transfer Rate */
#define OSPI_DTR_TRANSFER                 2

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_OSPI_H_ */
