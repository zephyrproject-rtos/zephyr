# SPDX-License-Identifier: Apache-2.0

board_runner_args(stm32cubeprogrammer "--erase" "--port=swd" "--reset-mode=hw")

set(UNO_Q_OPENOCD_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/support/uno_q_openocd.py")
if(CMAKE_HOST_WIN32)
  set(UNO_Q_OPENOCD_WRAPPER "${PROJECT_BINARY_DIR}/uno_q_openocd.cmd")
  file(WRITE "${UNO_Q_OPENOCD_WRAPPER}"
    "@echo off\r\n\"${PYTHON_EXECUTABLE}\" \"${UNO_Q_OPENOCD_SCRIPT}\" %*\r\n"
    )
else()
  set(UNO_Q_OPENOCD_WRAPPER "${PROJECT_BINARY_DIR}/uno_q_openocd")
  file(WRITE "${UNO_Q_OPENOCD_WRAPPER}"
    "#!/bin/sh\nexec \"${PYTHON_EXECUTABLE}\" \"${UNO_Q_OPENOCD_SCRIPT}\" \"$@\"\n"
    )
  file(CHMOD "${UNO_Q_OPENOCD_WRAPPER}"
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )
endif()
set(OPENOCD "${UNO_Q_OPENOCD_WRAPPER}" CACHE FILEPATH "" FORCE)

board_runner_args(openocd "--tcl-port=6666")
board_runner_args(openocd --cmd-pre-init "gdb_report_data_abort enable")
board_runner_args(openocd "--no-halt")
board_runner_args(openocd "--file-type=elf")

board_runner_args(jlink "--device=STM32U585AI" "--reset-after-load")

# keep first
include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
# FIXME: openocd runner requires use of STMicro openocd fork.
# Check board documentation for more details.
include(${ZEPHYR_BASE}/boards/common/openocd-stm32.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
