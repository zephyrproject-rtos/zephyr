# SPDX-License-Identifier: Apache-2.0

# Once we have more than 10 devices we should consider splitting them into
# subdirectories to match the drivers/ structure.
zephyr_library_sources_ifdef(CONFIG_EMUL_EEPROM_AT2X	emul_atmel_at24.c)
