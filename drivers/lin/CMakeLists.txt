# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/lin.h)

zephyr_library()

# LIN subsystem common files
# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_USERSPACE lin_handlers.c)
# zephyr-keep-sorted-stop

# LIN subsystem driver files
# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_LIN_SKELETON lin_skeleton.c)
# zephyr-keep-sorted-stop

add_subdirectory(renesas/ra)
add_subdirectory(transceiver)
