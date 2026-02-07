board_runner_args(jlink "--device=SF32LB58X" "--iface=swd")
board_runner_args(sftool "--chip=SF32LB58")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/sftool.board.cmake)
