# SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
# SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

set(template_dir ${ZEPHYR_HAL_INFINEON_MODULE_DIR}/mtb-template-psc3m8)

set(cat1b_dir ${template_dir}/files/templates)

# Add support for PSC3M8
if(CONFIG_SOC_DIE_PSC3M8)
  zephyr_include_directories(${cat1b_dir}/COMPONENT_MTB)
  zephyr_include_directories(${cat1b_dir}/COMPONENT_MTB/COMPONENT_CM33/HEADER_FILES)
  zephyr_library_sources(${cat1b_dir}/COMPONENT_MTB/COMPONENT_CM33/system_psc3_p8.c)
endif()
