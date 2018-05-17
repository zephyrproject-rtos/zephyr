# Use the TI OpenOCD (by default in /usr/local/openocd)
# See the Zephyr project CC3220SF_LAUNCHXL documentation on
# flashing prerequisites.
set(OPENOCD "/usr/local/bin/openocd" CACHE FILEPATH "" FORCE)
set(OPENOCD_DEFAULT_PATH ${OPENOCD}/scripts)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
