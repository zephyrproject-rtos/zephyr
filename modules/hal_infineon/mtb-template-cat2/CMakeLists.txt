# Copyright (c) 2025 Infineon Technologies AG,
# or an affiliate of Infineon Technologies AG.
#
# SPDX-License-Identifier: Apache-2.0

set(template_dir ${ZEPHYR_HAL_INFINEON_MODULE_DIR}/mtb-template-cat2)
set(cat2_dir ${template_dir}/files/templates)

if(CONFIG_SOC_FAMILY_INFINEON_CAT2)
    zephyr_include_directories(${cat2_dir}/COMPONENT_MTB)
    zephyr_library_sources(${cat2_dir}/COMPONENT_MTB/system_cat2.c)
endif()
