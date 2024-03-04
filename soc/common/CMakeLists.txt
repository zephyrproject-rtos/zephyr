# Copyright (c) 2024 Nordic Semiconductor ASA
# Copyright (c) 2024 sensry.io
# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_RISCV_OPENSBI riscv-opensbi)
add_subdirectory_ifdef(CONFIG_RISCV_PRIVILEGED riscv-privileged)
