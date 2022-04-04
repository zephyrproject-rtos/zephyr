# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_MCUBOOT_IMG_MANAGER boot)
add_subdirectory_ifdef(CONFIG_MCUBOOT_IMG_MANAGER img_util)
add_subdirectory(boot_mode)
