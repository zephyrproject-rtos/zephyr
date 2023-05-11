board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")

set(OPENOCD "/local/mcu/tools/openocd/src/openocd" CACHE FILEPATH "" FORCE)
set(OPENOCD_DEFAULT_PATH /local/mcu/tools/openocd/tcl)

include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
