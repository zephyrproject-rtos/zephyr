# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

# Override the binary used by qemu
set(QEMU_KERNEL_OPTION "-device;loader,file=${CMAKE_BINARY_DIR}/tfm_qemu.hex")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(tfm_psa_level_1)

# Source files in this sample
target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/psa_attestation.c)
target_sources(app PRIVATE src/psa_crypto.c)
target_sources(app PRIVATE src/shell.c)
target_sources(app PRIVATE src/tfm_ipc.c)
target_sources(app PRIVATE src/util_app_cfg.c)
target_sources(app PRIVATE src/util_app_log.c)
target_sources(app PRIVATE src/util_sformat.c)
