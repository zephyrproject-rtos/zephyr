# Copyright (c) 2024 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)
zephyr_sources_ifdef(CONFIG_SOC_SILABS_IMAGE_PROPERTIES soc_image_properties.c)
zephyr_linker_sources_ifdef(CONFIG_SOC_SILABS_IMAGE_PROPERTIES RODATA soc_image_properties.ld)
