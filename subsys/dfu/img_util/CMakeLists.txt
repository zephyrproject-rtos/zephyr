# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_MCUBOOT_IMG_MANAGER flash_img.c)

zephyr_library_link_libraries(MCUBOOT_BOOTUTIL)
