# Copyright (c) 2023 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_EDAC_INTEL_SOCFPGA edac.c)
zephyr_library_sources_ifdef(CONFIG_EDAC_SHELL_INTEL_SOCFPGA edac_shell.c)

if (CONFIG_EDAC_INTEL_SOCFPGA)
    zephyr_library_sources_ifdef(CONFIG_HPS_ECC_INTEL_SOCFPGA hps_ecc.c)

    zephyr_library_sources_ifdef(CONFIG_IO96B_INTEL_SOCFPGA io96b_ecc.c)
endif()
