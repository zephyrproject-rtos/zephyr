# SPDX-License-Identifier: Apache-2.0

set(BOARD_FLASH_RUNNER pyocd.sh)
set(BOARD_DEBUG_RUNNER pyocd.sh)

set(PYOCD_TARGET nrf52)

set_property(GLOBAL APPEND PROPERTY FLASH_SCRIPT_ENV_VARS
  PYOCD_TARGET
  )
