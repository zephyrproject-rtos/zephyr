set_ifndef(STLINK_FW stlink)

if(STLINK_FW STREQUAL jlink)
  board_runner_args(jlink "--device=stm32f070rb" "--speed=4000")
  include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
elseif(STLINK_FW STREQUAL stlink)
  include($ENV{ZEPHYR_BASE}/boards/common/openocd.board.cmake)
endif()
