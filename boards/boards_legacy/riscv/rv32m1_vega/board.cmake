# SPDX-License-Identifier: Apache-2.0

set(OPENOCD_USE_LOAD_IMAGE NO)

if(CONFIG_SOC_OPENISA_RV32M1_RI5CY)
board_runner_args(openocd "--config=${BOARD_DIR}/support/openocd_rv32m1_vega_ri5cy.cfg")
elseif(CONFIG_SOC_OPENISA_RV32M1_ZERO_RISCY)
board_runner_args(openocd "--config=${BOARD_DIR}/support/openocd_rv32m1_vega_zero_riscy.cfg")
endif()

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
