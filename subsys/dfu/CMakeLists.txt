# SPDX-License-Identifier: Apache-2.0

zephyr_library()

if(CONFIG_MCUBOOT_IMG_MANAGER)
    zephyr_library_sources(
        boot/mcuboot.c
        img_util/flash_img.c
    )
    zephyr_library_sources_ifdef(CONFIG_MCUBOOT_SHELL boot/mcuboot_shell.c)
    zephyr_library_link_libraries(MCUBOOT_BOOTUTIL)
endif()
