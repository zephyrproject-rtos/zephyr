# Copyright (c) 2026 StructCore Embedded
#
# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_SNMP
  snmp_ber.c
  snmp_core.c
  snmp_mib.c
  snmp_mib2.c
  snmp_service.c
)

zephyr_sources_ifdef(CONFIG_SNMP_VERSION_3
  snmp_usm.c
  snmp_vacm.c
)

zephyr_sources_ifdef(CONFIG_SNMP_TRAP_ENABLED
  snmp_trap.c
)
