# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(regulator_tps6521x)

target_sources(app PRIVATE src/emul_tps6521x.c)

if(CONFIG_TPS6521X_TEST_ID_MISMATCH)
  target_sources(app PRIVATE src/id_mismatch.c)
else()
  target_sources(app PRIVATE src/main.c)
endif()
