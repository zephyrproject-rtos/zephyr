# SPDX-License-Identifier: Apache-2.0

board_set_flasher_ifnset(pyocd.sh)
board_set_debugger_ifnset(pyocd.sh)

set(PYOCD_TARGET nrf52)

set_property(GLOBAL APPEND PROPERTY FLASH_SCRIPT_ENV_VARS
  PYOCD_TARGET
  )
