# Copyright (c) 2025 Core Devices LLC
# Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)
zephyr_sources(soc.c)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")

set(SF32LB52X_FTAB_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/../gen_ftab.py)
set(SF32LB52X_FTAB_HEX ${PROJECT_BINARY_DIR}/sf32lb52x_ftab.hex)

set(SF32LB52X_FTAB_GEN_COMMAND
  ${CMAKE_COMMAND} -E env ZEPHYR_BASE=${ZEPHYR_BASE}
  ${PYTHON_EXECUTABLE} ${SF32LB52X_FTAB_SCRIPT}
  --edt-pickle ${EDT_PICKLE}
  --output ${SF32LB52X_FTAB_HEX}
)

set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND ${SF32LB52X_FTAB_GEN_COMMAND})

string(MAKE_C_IDENTIFIER sftool SF32LB52X_SFTOOL_RUNNER_ID)
set_property(GLOBAL APPEND PROPERTY BOARD_RUNNER_ARGS_${SF32LB52X_SFTOOL_RUNNER_ID}
  --flash-file=${SF32LB52X_FTAB_HEX})
