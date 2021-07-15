# SPDX-License-Identifier: Apache-2.0

board_set_flasher_ifnset(openocd)
board_set_debugger_ifnset(openocd)

# "load_image" or "flash write_image erase"?
if(CONFIG_X86 OR CONFIG_ARC)
  set_ifndef(OPENOCD_USE_LOAD_IMAGE YES)
endif()
if(OPENOCD_USE_LOAD_IMAGE)
  set_ifndef(OPENOCD_FLASH load_image)
else()
  set_ifndef(OPENOCD_FLASH "flash write_image erase")
endif()

set(OPENOCD_CMD_LOAD_DEFAULT "${OPENOCD_FLASH}")
set(OPENOCD_CMD_VERIFY_DEFAULT "verify_image")

# Cortex-M-specific halt debug enable bit clearing:
# OpenOCD sets DHCSR.DEBUGEN to 1 to halt and flash the target.
# As openOCD shutdown does not clear DEBUGEN, we introduce a runner
# option to explicitly clear DEBUGEN (Halt Debug Enable bit) after
# openOCD has finished flashing and resetting the target (but before
# it shuts down).
if(CONFIG_CPU_CORTEX_M)
set(OPENOCD_CMD_HALT_DEBUG_EN_CLEAR "mww 0xE000EDF0 0xA05F0000")
endif()

board_finalize_runner_args(openocd
  --cmd-load "${OPENOCD_CMD_LOAD_DEFAULT}"
  --cmd-verify "${OPENOCD_CMD_VERIFY_DEFAULT}"
  --cmd-post-reset "${OPENOCD_CMD_HALT_DEBUG_EN_CLEAR}"
  )
