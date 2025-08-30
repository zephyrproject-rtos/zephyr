# Copyright (c) 2025 Andes Technology Corporation
# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_RISCV_CUSTOM_CSR_ANDES andes)
add_subdirectory_ifdef(CONFIG_RISCV_CUSTOM_CSR_CVA6 openhwgroup/cva6)
add_subdirectory_ifdef(CONFIG_RISCV_CUSTOM_CSR_NUCLEI nuclei)
add_subdirectory_ifdef(CONFIG_RISCV_CUSTOM_CSR_RI5CY openisa/ri5cy)
add_subdirectory_ifdef(CONFIG_RISCV_CUSTOM_CSR_ZERO_RISCY openisa/zero_riscy)
