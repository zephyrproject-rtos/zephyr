# Copyright (c) 2023 Cypress Semiconductor Corporation (an Infineon company) or
# an affiliate of Cypress Semiconductor Corporation
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_FAMILY_INFINEON_EDGE)
  set(template_dir ${ZEPHYR_HAL_INFINEON_MODULE_DIR}/mtb-template-pse8xxgp)
else()
  set(template_dir ${ZEPHYR_HAL_INFINEON_MODULE_DIR}/mtb-template-cat1)
endif()

set(cat1a_dir ${template_dir}/files/templates/cat1a)
set(cat1b_dir ${template_dir}/files/templates/cat1b)
set(cat1c_dir ${template_dir}/files/templates/cat1c)
set(edge_dir ${template_dir}/files)

# Add support for PSOC6 (CAT1A)
if(CONFIG_SOC_FAMILY_INFINEON_CAT1A)
  zephyr_include_directories(${cat1a_dir}/COMPONENT_MTB)
  zephyr_include_directories(${cat1a_dir}/COMPONENT_MTB/COMPONENT_CM33/HEADER_FILES)

  zephyr_library_sources_ifdef(CONFIG_CPU_CORTEX_M4
    ${cat1a_dir}/COMPONENT_MTB/COMPONENT_CM4/system_psoc6_cm4.c)
  zephyr_library_sources_ifdef(CONFIG_CPU_CORTEX_M0PLUS
    ${cat1a_dir}/COMPONENT_MTB/COMPONENT_CM0P/system_psoc6_cm0plus.c)
endif()

# Add support for CYW20829 (CAT1B)
if(CONFIG_SOC_FAMILY_INFINEON_CAT1B)
  zephyr_include_directories(${cat1b_dir}/COMPONENT_MTB)
  zephyr_include_directories(${cat1b_dir}/COMPONENT_MTB/COMPONENT_CM33/HEADER_FILES)
  zephyr_library_sources(${cat1b_dir}/COMPONENT_MTB/COMPONENT_CM33/system_cyw20829.c)
  zephyr_library_sources(${cat1b_dir}/COMPONENT_MTB/COMPONENT_CM33/system_psc3.c)
endif()

# Add support for XMC7x (CAT1C)
if(CONFIG_SOC_FAMILY_INFINEON_CAT1C)
  zephyr_include_directories(${cat1c_dir}/COMPONENT_MTB)
  zephyr_include_directories(${cat1c_dir}/COMPONENT_MTB/HEADER_FILES)
  zephyr_library_sources_ifdef(CONFIG_CPU_CORTEX_M0PLUS ${cat1c_dir}/COMPONENT_MTB/COMPONENT_CM0P/system_cm0plus.c)
  zephyr_library_sources_ifdef(CONFIG_CPU_CORTEX_M7 ${cat1c_dir}/COMPONENT_MTB/COMPONENT_CM7/system_cm7.c)
endif()

# Add support for psoce84 (EDGE)
if(CONFIG_SOC_FAMILY_INFINEON_EDGE)
  zephyr_include_directories(${edge_dir})
  zephyr_include_directories(${edge_dir}/devices/include)

  zephyr_library_sources(${edge_dir}/system_edge.c)
  zephyr_library_sources_ifdef(CONFIG_CPU_CORTEX_M33
    ${edge_dir}/COMPONENT_CM33/COMPONENT_SECURE_DEVICE/s_system_pse84.c)
  zephyr_include_directories_ifdef(CONFIG_CPU_CORTEX_M33
    ${edge_dir}/COMPONENT_CM33/COMPONENT_SECURE_DEVICE)
  zephyr_library_sources_ifdef(CONFIG_CPU_CORTEX_M55
    ${edge_dir}/COMPONENT_CM55/COMPONENT_NON_SECURE_DEVICE/ns_system_pse84.c)
endif()
