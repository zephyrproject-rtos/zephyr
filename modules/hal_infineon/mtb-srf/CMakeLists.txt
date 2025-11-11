# Copyright (c) 2025 Infineon Technologies AG,
# or an affiliate of Infineon Technologies AG.
#
# SPDX-License-Identifier: Apache-2.0

set(mtb_srf_dir ${ZEPHYR_HAL_INFINEON_MODULE_DIR}/mtb-srf)

zephyr_include_directories(${mtb_srf_dir}/include)
zephyr_include_directories(${mtb_srf_dir}/include/COMPONENT_NON_SECURE_DEVICE)
zephyr_include_directories(${mtb_srf_dir}/export)
