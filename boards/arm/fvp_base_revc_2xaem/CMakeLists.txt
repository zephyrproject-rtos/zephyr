# SPDX-License-Identifier: Apache-2.0

# Add atomic operations workaround with DMB barriers for SMP configurations
if(CONFIG_ATOMIC_OPERATIONS_ARCH)
  zephyr_library()
  zephyr_library_sources(atomic.c)
endif()
