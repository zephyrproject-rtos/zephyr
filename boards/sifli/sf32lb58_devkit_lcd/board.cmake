board_runner_args(jlink "--device=SF32LB58X" "--iface=swd")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
