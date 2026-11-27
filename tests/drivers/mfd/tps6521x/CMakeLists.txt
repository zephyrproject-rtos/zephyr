# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mfd_tps6521x)

# the register emulator is shared with the regulator test
set(EMUL_DIR ${ZEPHYR_BASE}/tests/drivers/regulator/tps6521x/src)

target_sources(app PRIVATE src/main.c ${EMUL_DIR}/emul_tps6521x.c)
target_include_directories(app PRIVATE ${EMUL_DIR})
target_compile_definitions(app PRIVATE EMUL_TPS6521X_BOOT_FLAGS)

if(EMUL_PIN_IS_EN)
  target_compile_definitions(app PRIVATE EMUL_TPS6521X_PIN_IS_EN)
endif()
