# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(spi_loopback)

target_sources(app PRIVATE src/spi.c)
target_sources_ifdef(CONFIG_SPI_RTIO app PRIVATE src/spi_rtio.c)
