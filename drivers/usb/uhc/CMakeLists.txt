# Copyright (c) 2022 Nordic Semiconductor
# Copyright 2024 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(uhc_common.c)
zephyr_library_sources_ifdef(CONFIG_UHC_MAX3421E uhc_max3421e.c)
zephyr_library_sources_ifdef(CONFIG_UHC_VIRTUAL uhc_virtual.c)
zephyr_library_sources_ifdef(CONFIG_UHC_NXP_EHCI uhc_mcux_common.c uhc_mcux_ehci.c)
zephyr_library_sources_ifdef(CONFIG_UHC_NXP_KHCI uhc_mcux_common.c uhc_mcux_khci.c)
zephyr_library_sources_ifdef(CONFIG_UHC_NXP_OHCI uhc_mcux_common.c uhc_mcux_ohci.c)
zephyr_library_sources_ifdef(CONFIG_UHC_NXP_IP3516HS uhc_mcux_common.c uhc_mcux_ip3516hs.c)
