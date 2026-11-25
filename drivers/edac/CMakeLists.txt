# Copyright (c) 2020 Intel Corporation
# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_EDAC_SHELL shell.c)

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_EDAC_IBECC edac_ibecc.c)
zephyr_library_sources_ifdef(CONFIG_EDAC_NXP_ERM edac_mcux_erm.c)
zephyr_library_sources_ifdef(CONFIG_EDAC_NXP_MECC edac_mcux_mecc.c)
zephyr_library_sources_ifdef(CONFIG_EDAC_SYNOPSYS edac_synopsys.c)
zephyr_library_sources_ifdef(CONFIG_EDAC_TI_K3_ECC_AGGR edac_ti_k3_ecc_aggr.c)
# zephyr-keep-sorted-stop
