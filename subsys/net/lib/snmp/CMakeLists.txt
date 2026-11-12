# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(
  snmp_ber.c
  snmp_registry.c
  snmp_agent.c
  snmp_transport.c
)
zephyr_library_sources_ifdef(CONFIG_SNMP_NOTIFICATIONS snmp_notify.c)
zephyr_library_sources_ifdef(CONFIG_SNMP_MIB2_SYSTEM mib2_system.c)
zephyr_library_sources_ifdef(CONFIG_SNMP_MIB2_SNMP mib2_snmp.c)

zephyr_linker_sources(ROM_SECTIONS iterables_snmp.ld)
