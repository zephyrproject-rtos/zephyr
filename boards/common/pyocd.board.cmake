# SPDX-License-Identifier: Apache-2.0

# Cortex-M-specific halt debug enable bit clearing:
# OpenOCD sets DHCSR.DEBUGEN to 1 to halt and flash the target.
# As openOCD shutdown does not clear DEBUGEN, we introduce a runner
# option to explicitly clear DEBUGEN (Halt Debug Enable bit) after
# openOCD has finished flashing and resetting the target (but before
# it shuts down).
if(CONFIG_CPU_CORTEX_M)
set(PYOCD_CMD_HALT_DEBUG_EN_CLEAR "write32 0xE000EDF0 0xA05F0000")
endif()


board_set_flasher_ifnset(pyocd)
board_set_debugger_ifnset(pyocd)
board_finalize_runner_args(pyocd "--dt-flash=y"
    --post-flash "${PYOCD_CMD_HALT_DEBUG_EN_CLEAR}")
