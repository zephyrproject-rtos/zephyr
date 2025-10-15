# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_FUSE_BSEC_STM32 fuse_bsec_stm32.c)
