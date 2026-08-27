# SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
# or an affiliate of Infineon Technologies AG. All rights reserved.</text>
#
# SPDX-License-Identifier: Apache-2.0

set(pse84_template_dir ${ZEPHYR_HAL_INFINEON_MODULE_DIR}/mtb-template-pse8xxgp/files)
set(pse84_eval_cm33_dir ${pse84_template_dir}/COMPONENT_CM33)

zephyr_include_directories(${pse84_template_dir})
zephyr_include_directories(${pse84_template_dir}/devices/include)

zephyr_library_sources(${pse84_template_dir}/system_edge.c)

if(CONFIG_CPU_CORTEX_M33)
  if(CONFIG_TRUSTED_EXECUTION_NONSECURE)
    zephyr_library_sources(${pse84_eval_cm33_dir}/COMPONENT_NON_SECURE_DEVICE/ns_system_pse84.c)
  else()
    set(pse84_eval_cm33_secure_dir ${pse84_eval_cm33_dir}/COMPONENT_SECURE_DEVICE)
    zephyr_include_directories(${pse84_eval_cm33_secure_dir})
    zephyr_library_sources(${pse84_eval_cm33_secure_dir}/s_system_pse84.c)
  endif()
endif()

zephyr_library_sources_ifdef(CONFIG_CPU_CORTEX_M55
  ${pse84_template_dir}/COMPONENT_CM55/COMPONENT_NON_SECURE_DEVICE/ns_system_pse84.c)
