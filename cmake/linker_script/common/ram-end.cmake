# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-FileCopyrightText: Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
# SPDX-License-Identifier: Apache-2.0

zephyr_linker_section(NAME .last_ram_section VMA RAM LMA RAM_REGION LINK_IN NOLOAD TYPE BSS)
zephyr_linker_section_configure(
  SECTION .last_ram_section
  INPUT ""
  SYMBOLS _end z_mapped_end
  KEEP
  )
