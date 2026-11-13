# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")

# The A7-side Linux remoteproc driver expects a .resource_table section in
# the ELF (see samples/subsys/ipc/openamp_rsc_table).  It must be a standalone
# output section: Linux finds it by name, so placing it inside .rodata would
# hide it.  KEEP prevents --gc-sections from dropping it.
zephyr_linker_sources_ifdef(CONFIG_OPENAMP_RSC_TABLE
  ROM_SECTIONS
  rsc_table.ld
)
