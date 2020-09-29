# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_EMUL		emul.c)
add_subdirectory(i2c)
