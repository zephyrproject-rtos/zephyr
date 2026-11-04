# SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
# SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
# SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(soc_m7.c)
zephyr_library_sources_ifdef(CONFIG_ATOMIC_OPERATIONS_ARCH atomic_ipc.c)
if(CONFIG_SMP OR (CONFIG_MP_MAX_NUM_CPUS GREATER 1))
  zephyr_library_sources(smp.c)
endif()
