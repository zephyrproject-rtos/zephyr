# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(acpi.c)
zephyr_library_sources_ifdef(CONFIG_ACPI_SHELL acpi_shell.c)
