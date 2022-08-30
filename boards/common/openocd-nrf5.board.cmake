# SPDX-License-Identifier: Apache-2.0

# Infer nrf51 vs nrf52 etc from the BOARD name. This enforces a board
# naming convention: "nrf5x" must appear somewhere in the board name
# for this to work.
#
# Boards which don't meet this convention can set this variable before
# including this script.
if (NOT DEFINED OPENOCD_NRF5_SUBFAMILY)
  string(REGEX MATCH nrf5. OPENOCD_NRF5_SUBFAMILY "${BOARD}")
endif()
if("${OPENOCD_NRF5_SUBFAMILY}" STREQUAL "")
  message(FATAL_ERROR
    "Can't match nrf5 subfamily from BOARD name. "
    "To fix, set CMake variable OPENOCD_NRF5_SUBFAMILY.")
endif()

if (NOT DEFINED OPENOCD_NRF5_INTERFACE)
  set(OPENOCD_NRF5_INTERFACE "jlink")
endif()

# We can do the right thing for supported subfamilies using a generic
# script, at least for openocd 0.10.0 and the openocd shipped with
# Zephyr SDK 0.10.3.
set(pre_init_cmds
  "set WORKAREASIZE 0x4000"	# 16 kB RAM used for flashing
  "source [find interface/${OPENOCD_NRF5_INTERFACE}.cfg]"
  "transport select swd"
  "source [find target/${OPENOCD_NRF5_SUBFAMILY}.cfg]"
)

foreach(cmd ${pre_init_cmds})
  board_runner_args(openocd --cmd-pre-init "${cmd}")
endforeach()

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
