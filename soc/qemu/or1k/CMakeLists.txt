#
# Copyright (c) 2025 NVIDIA Corporation <jholdsworth@nvidia.com>
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_compile_options(
  ${TOOLCHAIN_C_FLAGS}
)

zephyr_ld_options(
  ${TOOLCHAIN_LD_FLAGS}
)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/openrisc/linker.ld CACHE INTERNAL "")
