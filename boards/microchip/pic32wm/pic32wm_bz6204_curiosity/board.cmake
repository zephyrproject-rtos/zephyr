# SPDX-FileCopyrightText: Copyright (c) 2026 Microchip Technology Inc.
# SPDX-License-Identifier: Apache-2.0

# NOTE: PIC32WM_BZ6204 OpenOCD support is not available in public OpenOCD releases.
# For Zephyr SDK v0.17.x: adding the custom OpenOCD path to the system PATH
# environment variable is sufficient — no need to uncomment the lines below.
# For Zephyr SDK v1.0.0 and above: the SDK bundles its own OpenOCD which takes
# precedence over the system PATH. To override it with a custom OpenOCD, you
# must uncomment the two lines below and replace <openocd-install-dir> with
# the path to your custom OpenOCD installation.

# set(OPENOCD "<openocd-install-dir>/bin/openocd.exe" CACHE FILEPATH "" FORCE)
# set(OPENOCD_DEFAULT_PATH "<openocd-install-dir>/share/openocd/scripts" CACHE PATH "" FORCE)

board_runner_args(mplab_ipe "--tool" "PKOB4" "--part" "32WM_BZ6204" "--hex-file=${CMAKE_BINARY_DIR}/zephyr/zephyr_signed.hex" "--erase" "--verify")
board_runner_args(openocd
  "--hex-file=${CMAKE_BINARY_DIR}/zephyr/zephyr_signed.hex"
)
board_runner_args(jlink "--device=pic32wm_bz6204" "--speed=4000")

include(${ZEPHYR_BASE}/boards/common/mplab_ipe.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
