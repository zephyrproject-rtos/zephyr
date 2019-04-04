# SPDX-License-Identifier: Apache-2.0

 zephyr_library()

zephyr_library_sources_ifdef(CONFIG_SI7006 si7006.c)
