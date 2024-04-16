# Copyright (c) 2023 Cypress Semiconductor Corporation (an Infineon company) or
# an affiliate of Cypress Semiconductor Corporation
#
# SPDX-License-Identifier: Apache-2.0

set(template_dir ${ZEPHYR_HAL_INFINEON_MODULE_DIR}/mtb-template-cat1)
set(cat1a_dir ${template_dir}/files/templates/cat1a)

zephyr_include_directories(${cat1a_dir}/COMPONENT_MTB)

zephyr_library_sources_ifdef(CONFIG_CPU_CORTEX_M4
	${cat1a_dir}/COMPONENT_MTB/COMPONENT_CM4/system_psoc6_cm4.c)
zephyr_library_sources_ifdef(CONFIG_CPU_CORTEX_M0PLUS
	${cat1a_dir}/COMPONENT_MTB/COMPONENT_CM0P/system_psoc6_cm0plus.c)
