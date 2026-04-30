# SPDX-License-Identifier: Apache-2.0

# Add atomic operations workaround with DMB barriers for SMP configurations.
# See boards/arm/fvp_base_revc_2xaem/CMakeLists.txt for details.
if(CONFIG_ATOMIC_OPERATIONS_ARCH)
  zephyr_library()
  zephyr_library_sources(atomic.c)
endif()
