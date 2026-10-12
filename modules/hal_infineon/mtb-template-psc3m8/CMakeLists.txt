# SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
# SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

set(template_dir ${ZEPHYR_HAL_INFINEON_MODULE_DIR}/mtb-template-psc3m8)

set(psc3m8_dir ${template_dir}/files)

zephyr_include_directories(
  ${psc3m8_dir}
  ${psc3m8_dir}/devices/include)
zephyr_library_sources(${psc3m8_dir}/system_psc3_p8.c)

# Skip the BSP's deep-sleep-RAM warmboot code: it needs the MTB startup file
# and linker script that Zephyr replaces.
zephyr_library_compile_definitions(CYBSP_SKIP_PM_REGISTRATION)
