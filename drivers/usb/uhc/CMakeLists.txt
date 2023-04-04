# Copyright (c) 2022 Nordic Semiconductor
# Copyright (c) 2023 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(uhc_common.c)
zephyr_library_sources_ifdef(CONFIG_UHC_MAX3421E uhc_max3421e.c)
zephyr_library_sources_ifdef(CONFIG_UHC_VIRTUAL uhc_virtual.c)
zephyr_library_sources_ifdef(CONFIG_UHC_DWC_HS uhc_dwc_hs.c)
