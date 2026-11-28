# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(arm_gicv3_its)

target_sources_ifndef(CONFIG_TEST_GICV3_ITS_SMALL_LPI_POOL app PRIVATE src/main.c)
target_sources_ifdef(CONFIG_TEST_GICV3_ITS_SMALL_LPI_POOL app PRIVATE src/exhaustion.c)
target_include_directories(app PRIVATE
  ${ARCH_DIR}/${ARCH}/include
)
