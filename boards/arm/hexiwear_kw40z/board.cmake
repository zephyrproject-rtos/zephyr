set_ifndef(OPENSDA_FW jlink)

if(OPENSDA_FW STREQUAL jlink)
  set_ifndef(BOARD_DEBUG_RUNNER jlink)
elseif(OPENSDA_FW STREQUAL daplink)
  set_ifndef(BOARD_DEBUG_RUNNER pyocd)
  set_ifndef(BOARD_FLASH_RUNNER pyocd)
endif()

board_runner_args(jlink "--device=MKW40Z160xxx4")
board_runner_args(pyocd "--target=kw40z4")

include($ENV{ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include($ENV{ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
