# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(
  snmp_ber.c
  snmp_registry.c
)

zephyr_linker_sources(ROM_SECTIONS iterables_snmp.ld)
