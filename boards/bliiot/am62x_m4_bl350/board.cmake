# board.cmake for am62x_m4_bl350

if(CONFIG_SOC_AM6234_M4)
  board_runner_args(openocd "--no-init" "--no-halt" "--no-targets" "--gdb-client-port=3339")
  include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
endif()
