# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_QEMU_FWCFG qemu_fwcfg.c)
zephyr_library_sources_ifdef(CONFIG_QEMU_FWCFG_MMIO qemu_fwcfg_mmio.c)
zephyr_library_sources_ifdef(CONFIG_QEMU_FWCFG_IOPORT qemu_fwcfg_ioport.c)
