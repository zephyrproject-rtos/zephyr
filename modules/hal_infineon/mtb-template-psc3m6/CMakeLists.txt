# SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
# SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

set(template_dir ${ZEPHYR_HAL_INFINEON_MODULE_DIR}/mtb-template-psc3m6)

set(psc3m6_dir ${template_dir}/files)

zephyr_include_directories(${psc3m6_dir})
zephyr_include_directories(${psc3m6_dir}/devices/include)
zephyr_library_sources(${psc3m6_dir}/system_psc3_x6.c)

# Skip the BSP's deep-sleep-RAM warmboot code: it needs the MTB startup file
# and linker script that Zephyr replaces.
zephyr_library_compile_definitions(CYBSP_SKIP_PM_REGISTRATION)
