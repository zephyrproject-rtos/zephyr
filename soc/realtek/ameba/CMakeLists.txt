# Copyright (c) 2024 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

# common/ is amebasmart-only glue (IRQ shims, reset shell command, KM0/KM4
# linker script).
if(CONFIG_SOC_SERIES_AMEBASMART)
  add_subdirectory(common)
endif()
add_subdirectory(${CONFIG_SOC_SERIES})
