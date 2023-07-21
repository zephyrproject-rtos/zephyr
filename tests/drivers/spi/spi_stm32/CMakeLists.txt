# Copyright (c) 2023 Graphcore Ltd.
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

project(spi_stm32_test)

find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})

include_directories(
  ./include
  ${ZEPHYR_BASE}/drivers/spi
  ${ZEPHYR_BASE}/soc/arm/st_stm32/common
  ${ZEPHYR_BASE}/subsys/testsuite/include/
)

target_sources(testbinary
  PRIVATE
    src/main.c
    src/spi_setup.c
    mocks/src/stm32_spi_iface_mocks.c
    mocks/src/pinctrl_mocks.c
    mocks/src/k_mutex_mocks.c
    mocks/src/kernel_mocks.c
    mocks/src/gpio_mocks.c
    mocks/src/arch_interface_mocks.c
    ${ZEPHYR_BASE}/drivers/spi/spi_ll_stm32.c
)

target_compile_options(testbinary
  PRIVATE
    -Werror
    -Wall
    -O0
    -g3
    -ggdb
)
