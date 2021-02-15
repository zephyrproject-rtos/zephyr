# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_ARM_PSCI	psci.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE	psci_handlers.c)
