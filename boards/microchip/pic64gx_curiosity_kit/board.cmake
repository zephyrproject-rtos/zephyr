# SPDX-License-Identifier: Apache-2.0

# The openocd runner loads zephyr.elf into DDR over the on-board JTAG once the
# Hart Software Services (HSS) have initialized DDR, see support/openocd.cfg.
board_runner_args(openocd --file-type=elf --no-load)

if(CONFIG_SOC_PIC64GX1000_E51)
  board_runner_args(openocd --gdb-client-port=3333
                    "--cmd-reset-halt=pic64gx_hss_reset_halt e51")
else()
  board_runner_args(openocd --gdb-client-port=3334
                    "--cmd-reset-halt=pic64gx_hss_reset_halt u54_1")
endif()

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
