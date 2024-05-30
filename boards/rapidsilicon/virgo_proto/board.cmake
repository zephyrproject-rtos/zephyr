# SPDX-License-Identifier: Apache-2.0
# Note: Please change the --elf-file path,
# server ip and port as per your machine.

board_runner_args(
                    openocd 
                    "--cmd-load=load"
                    "--use-elf"  
                    "--elf-file=${ZEPHYR_BASE}/build/zephyr/zephyr.elf" 
                    "--gdb-init=target extended-remote 10.0.8.102:3333"
                 )
set(OPENOCD_FLASH "flash")
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
