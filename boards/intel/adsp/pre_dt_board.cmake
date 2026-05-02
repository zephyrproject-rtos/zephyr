# Copyright (c) 2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

if(NOT CONFIG_BOARD_INTEL_ADSP_ACE20_LNL)

# Suppress "unique_unit_address_if_enabled" to handle the following overlaps:
# - dmic0: dmic0@10000 & dmic1: dmic1@10000
list(APPEND EXTRA_DTC_FLAGS "-Wno-unique_unit_address_if_enabled")

endif()
