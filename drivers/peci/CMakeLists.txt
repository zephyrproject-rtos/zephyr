# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_PECI_XEC	peci_mchp_xec.c)
zephyr_library_sources_ifdef(CONFIG_PECI_ITE_IT8XXX2	peci_ite_it8xxx2.c)
zephyr_library_sources_ifdef(CONFIG_PECI_NPCX	peci_npcx.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE   peci_handlers.c)
