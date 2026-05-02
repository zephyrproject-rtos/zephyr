# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_property(ALLOW_EMPTY TRUE)
zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/haptics.h)

zephyr_library_sources_ifdef(CONFIG_HAPTICS_SHELL haptics_shell.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE haptics_handlers.c)

# zephyr-keep-sorted-start
add_subdirectory(cirrus)
add_subdirectory(ti)
# zephyr-keep-sorted-stop
