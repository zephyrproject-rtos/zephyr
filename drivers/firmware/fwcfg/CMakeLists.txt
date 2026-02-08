# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_QEMU_FWCFG fwcfg.c fwcfg_mmio.c)
zephyr_library_sources_ifdef(CONFIG_X86 fwcfg_ioport.c)
