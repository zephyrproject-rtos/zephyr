# Copyright (c) 2023 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_EDAC_IBECC edac_ibecc)
add_subdirectory_ifdef(CONFIG_EDAC_INTEL_SOCFPGA edac_intel_socfpga)
