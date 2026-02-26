# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
    boot/dfu_boot_utils.c
    img_util/flash_img.c
)

if(CONFIG_MCUBOOT_IMG_MANAGER)
    zephyr_library_sources(
        boot/mcuboot.c
    )
    zephyr_library_sources_ifdef(CONFIG_MCUBOOT_SHELL boot/mcuboot_shell.c)
    zephyr_library_link_libraries(MCUBOOT_BOOTUTIL)
endif()

if(CONFIG_NXPBOOT_IMG_MANAGER)
    zephyr_library_sources(
        boot/nxp_boot.c
    )
    zephyr_library_link_libraries_ifdef(CONFIG_NXPBOOT_HYBRID_MCUBOOT MCUBOOT_BOOTUTIL)
endif()
