# SPDX-License-Identifier: Apache-2.0

if(CONFIG_MCUBOOT_IMG_MANAGER)
zephyr_library()
zephyr_library_sources(mcuboot.c)
zephyr_library_sources_ifdef(CONFIG_MCUBOOT_SHELL mcuboot_shell.c)
zephyr_library_link_libraries(MCUBOOT_BOOTUTIL)
endif()
