# SPDX-FileCopyrightText: Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_EBPF)
  zephyr_library()

  zephyr_library_sources(
    ebpf_init.c
    ebpf_vm.c
    ebpf_verifier.c
    ebpf_maps.c
    ebpf_helpers.c
    ebpf_prog.c
  )

  zephyr_library_sources_ifdef(CONFIG_EBPF_TRACING ebpf_tracing.c)
  zephyr_library_sources_ifdef(CONFIG_EBPF_SHELL   ebpf_shell.c)

  # Include kernel internals for _current access in tracing macros
  zephyr_library_include_directories(
    ${ZEPHYR_BASE}/kernel/include
  )
endif()
