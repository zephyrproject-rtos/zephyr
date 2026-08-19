# Copyright (c) 2026 BayLibre SAS
# SPDX-License-Identifier: Apache-2.0

# Sysbuild configuration for Corstone-1000 FVP
#
# This builds:
# 1. TF-M for the Secure Enclave (Cortex-M0+)
# 2. TF-A for the Host CPU (Cortex-A320) with Zephyr as BL33
# 3. Combined flash image for the FVP
#
# This board uses a custom ExternalProject-based build instead of the
# standard Zephyr TF-M sysbuild infrastructure (modules/trusted-firmware-m/)
# because the Corstone-1000 has a fundamentally different architecture:
#
# - TF-M runs on a *separate* Cortex-M0+ Secure Enclave, not on the same
#   CPU as Zephyr via TrustZone. This requires a different toolchain
#   (arm-zephyr-eabi for TF-M vs aarch64-zephyr-elf for Zephyr).
#
# - TF-A is needed as the host CPU firmware (BL2/BL31), which the standard
#   TF-M integration does not handle.
#
# - The flash image combines TF-M, TF-A (with signed BL2), and Zephyr
#   into a GPT-partitioned image specific to this platform.
#
# If/when another Zephyr board has similar dual-CPU topology, this logic
# should be generalized into shared infrastructure.
#
# This build infrastructure currently targets Linux/Unix host shells
# (uses sh, dd, sgdisk).  Windows host builds are not supported.

include(ExternalProject)

# Zephyr SDK is required for both the Cortex-M0+ (TF-M) and AArch64 (TF-A) toolchains.
find_package(Zephyr-sdk QUIET CONFIG HINTS $ENV{ZEPHYR_SDK_INSTALL_DIR})
if(NOT ZEPHYR_SDK_INSTALL_DIR AND DEFINED ENV{ZEPHYR_SDK_INSTALL_DIR})
  set(ZEPHYR_SDK_INSTALL_DIR $ENV{ZEPHYR_SDK_INSTALL_DIR})
endif()
if(NOT ZEPHYR_SDK_INSTALL_DIR)
  message(FATAL_ERROR "Zephyr SDK not found. "
    "Set ZEPHYR_SDK_INSTALL_DIR or install the SDK.")
endif()

# sgdisk is required for GPT partition creation in the SE flash image.
find_program(SGDISK sgdisk)
if(NOT SGDISK)
  message(FATAL_ERROR "sgdisk not found in PATH. "
    "Install the gdisk package (e.g. 'apt install gdisk' or "
    "'dnf install gdisk') and re-run cmake.")
endif()

# Get west topdir for calculating module paths
# This allows us to find modules even if module.yml is missing
execute_process(
  COMMAND west topdir
  OUTPUT_VARIABLE west_topdir
  OUTPUT_STRIP_TRAILING_WHITESPACE
  RESULT_VARIABLE west_result
)

if(NOT west_result EQUAL 0)
  # Fallback: assume standard west workspace layout
  get_filename_component(west_topdir "${ZEPHYR_BASE}/.." ABSOLUTE)
endif()

# Get module paths
# Try module variables first, fall back to west workspace layout
if(DEFINED ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR AND EXISTS "${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR}")
  set(tfm_source_dir ${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR})
else()
  set(tfm_source_dir ${west_topdir}/modules/tee/tf-m/trusted-firmware-m)
endif()

if(DEFINED ZEPHYR_TRUSTED_FIRMWARE_A_MODULE_DIR AND EXISTS "${ZEPHYR_TRUSTED_FIRMWARE_A_MODULE_DIR}")
  set(tfa_source_dir ${ZEPHYR_TRUSTED_FIRMWARE_A_MODULE_DIR})
else()
  set(tfa_source_dir ${west_topdir}/modules/tee/tf-a/trusted-firmware-a)
endif()

# mbedtls module path (used as MBEDCRYPTO_PATH for TF-M)
# TF-M requires mbedtls-3.6 because mbedtls 4.x split the crypto code into a
# separate tf-psa-crypto module that TF-M does not support yet.
# See west.yml: "Required for TF-M build until we bump it to v2.3"
if(DEFINED ZEPHYR_MBEDTLS_3_6_MODULE_DIR AND EXISTS "${ZEPHYR_MBEDTLS_3_6_MODULE_DIR}")
  set(mbedtls_module_dir ${ZEPHYR_MBEDTLS_3_6_MODULE_DIR})
else()
  set(mbedtls_module_dir ${west_topdir}/modules/crypto/mbedtls-3.6)
endif()

# CMSIS_6 module path
if(DEFINED ZEPHYR_CMSIS_6_MODULE_DIR AND EXISTS "${ZEPHYR_CMSIS_6_MODULE_DIR}")
  set(cmsis_6_module_dir ${ZEPHYR_CMSIS_6_MODULE_DIR})
else()
  set(cmsis_6_module_dir ${west_topdir}/modules/hal/cmsis_6)
endif()

# Verify the source directories exist
if(NOT EXISTS "${tfm_source_dir}")
  message(FATAL_ERROR "TF-M source directory not found: ${tfm_source_dir}\n"
    "Run 'west update' to fetch the module.")
endif()

if(NOT EXISTS "${tfa_source_dir}")
  message(FATAL_ERROR "TF-A source directory not found: ${tfa_source_dir}\n"
    "Run 'west update' to fetch the module.")
endif()

if(NOT EXISTS "${mbedtls_module_dir}")
  message(FATAL_ERROR "mbedtls module directory not found: ${mbedtls_module_dir}\n"
    "Run 'west update' to fetch the module.")
endif()

if(NOT EXISTS "${cmsis_6_module_dir}")
  message(FATAL_ERROR "CMSIS_6 module directory not found: ${cmsis_6_module_dir}\n"
    "Run 'west update' to fetch the module.")
endif()

message(STATUS "Corstone-1000: TF-M source: ${tfm_source_dir}")
message(STATUS "Corstone-1000: TF-A source: ${tfa_source_dir}")
message(STATUS "Corstone-1000: mbedtls: ${mbedtls_module_dir}")
message(STATUS "Corstone-1000: CMSIS_6: ${cmsis_6_module_dir}")

# Output directories
set(tfm_binary_dir ${CMAKE_BINARY_DIR}/tfm)
set(tfa_binary_dir ${CMAKE_BINARY_DIR}/tfa)

# The firmware output directory is exposed via the sysbuild cache so that
# the image-side board.cmake can locate it with zephyr_get() instead of
# assuming a particular build-folder layout.
set(CORSTONE1000_FIRMWARE_DIR ${CMAKE_BINARY_DIR}/firmware CACHE PATH
    "Corstone-1000 firmware output directory")

file(MAKE_DIRECTORY ${tfm_binary_dir})
file(MAKE_DIRECTORY ${tfa_binary_dir})
file(MAKE_DIRECTORY ${CORSTONE1000_FIRMWARE_DIR})

# =============================================================================
# TF-M Build for Secure Enclave (Cortex-M0+)
# =============================================================================

message(STATUS "Corstone-1000: Building TF-M for Secure Enclave")

# TF-M build type
if(SB_CONFIG_BOARD_CORSTONE1000_TFM_DEBUG)
  set(tfm_cmake_build_type "Debug")
else()
  set(tfm_cmake_build_type "Release")
endif()

# TF-M requires a Cortex-M toolchain (arm-zephyr-eabi from the Zephyr SDK).
# Pass the full path to CROSS_COMPILE as the standard Zephyr TF-M integration does.
set(tfm_toolchain_file ${tfm_source_dir}/toolchain_GNUARM.cmake)
# SDK >= 1.0 moved toolchains under gnu/
if(EXISTS "${ZEPHYR_SDK_INSTALL_DIR}/gnu/arm-zephyr-eabi")
  set(tfm_cross_compile "${ZEPHYR_SDK_INSTALL_DIR}/gnu/arm-zephyr-eabi/bin/arm-zephyr-eabi")
else()
  set(tfm_cross_compile "${ZEPHYR_SDK_INSTALL_DIR}/arm-zephyr-eabi/bin/arm-zephyr-eabi")
endif()

# TF-M CMake arguments
# Note: We must provide MBEDCRYPTO_PATH and CMSIS_PATH because TF-M's CMakeLists.txt
# processes bl2/ before lib/ext/mbedcrypto/, so the default "DOWNLOAD" value doesn't
# work on initial configure. Providing paths from Zephyr modules avoids the fetch.
# Map Kconfig log level to the three different TF-M log level systems:
# - TF-M BL1: LOG_LEVEL_{NONE,ERROR,WARNING,INFO,DEBUG}
# - MCUboot:  {OFF,ERROR,WARNING,INFO,DEBUG}
# - Platform: PLAT_LOG_LEVEL 0-4 (matches Kconfig directly)
# - Secure Partitions: TFM_PARTITION_LOG_LEVEL_{SILENCE,ERROR,INFO,DEBUG} (0-3)
if(SB_CONFIG_BOARD_CORSTONE1000_TFM_LOG_LEVEL EQUAL 0)
  set(tfm_log_level_str "LOG_LEVEL_NONE")
  set(mcuboot_log_level_str "OFF")
  set(tfm_sp_log_level_str "TFM_PARTITION_LOG_LEVEL_SILENCE")
elseif(SB_CONFIG_BOARD_CORSTONE1000_TFM_LOG_LEVEL EQUAL 1)
  set(tfm_log_level_str "LOG_LEVEL_ERROR")
  set(mcuboot_log_level_str "ERROR")
  set(tfm_sp_log_level_str "TFM_PARTITION_LOG_LEVEL_ERROR")
elseif(SB_CONFIG_BOARD_CORSTONE1000_TFM_LOG_LEVEL EQUAL 2)
  set(tfm_log_level_str "LOG_LEVEL_WARNING")
  set(mcuboot_log_level_str "WARNING")
  set(tfm_sp_log_level_str "TFM_PARTITION_LOG_LEVEL_INFO")
elseif(SB_CONFIG_BOARD_CORSTONE1000_TFM_LOG_LEVEL EQUAL 3)
  set(tfm_log_level_str "LOG_LEVEL_INFO")
  set(mcuboot_log_level_str "INFO")
  set(tfm_sp_log_level_str "TFM_PARTITION_LOG_LEVEL_INFO")
else()
  set(tfm_log_level_str "LOG_LEVEL_DEBUG")
  set(mcuboot_log_level_str "DEBUG")
  set(tfm_sp_log_level_str "TFM_PARTITION_LOG_LEVEL_DEBUG")
endif()

message(STATUS "Corstone-1000: TF-M log level: ${tfm_log_level_str}, MCUboot: ${mcuboot_log_level_str}")

set(tfm_cmake_args
  -DTFM_PLATFORM=arm/corstone1000
  -DPLATFORM_IS_FVP=TRUE
  -DCMAKE_BUILD_TYPE=${tfm_cmake_build_type}
  -DTFM_TOOLCHAIN_FILE=${tfm_toolchain_file}
  -DCROSS_COMPILE=${tfm_cross_compile}
  -DBL1=ON
  -DBL2=ON
  -DMCUBOOT_PATH=${ZEPHYR_MCUBOOT_MODULE_DIR}
  -DMBEDCRYPTO_PATH=${mbedtls_module_dir}
  -DCMSIS_PATH=${cmsis_6_module_dir}
  -DTFM_BL1_LOG_LEVEL=${tfm_log_level_str}
  -DMCUBOOT_LOG_LEVEL=${mcuboot_log_level_str}
  -DPLAT_LOG_LEVEL=${SB_CONFIG_BOARD_CORSTONE1000_TFM_LOG_LEVEL}
  -DTFM_PARTITION_LOG_LEVEL=${tfm_sp_log_level_str}
  -DCRYPTO_HW_ACCELERATOR=ON
  # Disable Initial Attestation: depends on QCBOR/t_cose which have licensing
  # issues and are not shipped with Zephyr (TF-M would try to clone them).
  -DTFM_PARTITION_INITIAL_ATTESTATION=OFF
  # Cortex-A320 variant with DSU-120T PPU power management
  -DCORSTONE1000_CORTEX_A320=TRUE
  -DCORSTONE1000_DSU_120T=TRUE
)

# Multi-core support: boot all 4 host CPUs (secondaries enter holding pen)
if("${BOARD_QUALIFIERS}" MATCHES "/smp$")
  list(APPEND tfm_cmake_args -DENABLE_MULTICORE=TRUE)
endif()

# STOPGAP: Corstone1000 TF-M platform code has implicit int/pointer casts
# and a missing declaration that GCC 14+ (Zephyr SDK 1.0+) treats as errors.
# Other TF-M platforms don't have these issues. Remove this workaround once
# the corstone1000 platform code is fixed upstream in TF-M.
# We use a wrapper toolchain file because TF-M's toolchain_GNUARM.cmake
# overwrites CMAKE_C_FLAGS, so flags passed via -DCMAKE_C_FLAGS are lost.
set(tfm_toolchain_wrapper ${CMAKE_CURRENT_BINARY_DIR}/toolchain_corstone1000.cmake)
file(WRITE ${tfm_toolchain_wrapper}
  "include(${tfm_toolchain_file})\n"
  "string(APPEND CMAKE_C_FLAGS \" -Wno-int-conversion -Wno-implicit-function-declaration -Wno-incompatible-pointer-types\")\n"
)
list(REMOVE_ITEM tfm_cmake_args "-DTFM_TOOLCHAIN_FILE=${tfm_toolchain_file}")
list(APPEND tfm_cmake_args "-DTFM_TOOLCHAIN_FILE=${tfm_toolchain_wrapper}")

ExternalProject_Add(
  tfm_secure_enclave
  SOURCE_DIR ${tfm_source_dir}
  BINARY_DIR ${tfm_binary_dir}
  CMAKE_ARGS ${tfm_cmake_args}
  BUILD_COMMAND ${CMAKE_COMMAND} --build . -- install
  INSTALL_COMMAND ""
  BUILD_ALWAYS True
  USES_TERMINAL_BUILD True
)

# Post-build: Create bl1.bin from TF-M outputs
# bl1.bin = bl1_1.bin + bl1_provisioning_bundle.bin
# Offset must match BL1_1_CODE_SIZE from region_defs.h (0xE800 = 59392)
add_custom_command(
  OUTPUT ${CORSTONE1000_FIRMWARE_DIR}/bl1.bin
  COMMAND ${CMAKE_COMMAND} -E copy ${tfm_binary_dir}/bin/bl1_1.bin ${CORSTONE1000_FIRMWARE_DIR}/bl1.bin
  COMMAND dd conv=notrunc bs=1 if=${tfm_binary_dir}/bin/bl1_provisioning_bundle.bin of=${CORSTONE1000_FIRMWARE_DIR}/bl1.bin seek=59392 2>/dev/null
  DEPENDS tfm_secure_enclave
  COMMENT "Creating Secure Enclave ROM image (bl1.bin)"
)

add_custom_target(tfm_bl1 ALL DEPENDS ${CORSTONE1000_FIRMWARE_DIR}/bl1.bin)

# Post-build: Create Secure Enclave flash image with FIP partition
# The base TF-M script creates partitions 1-10, we need to add FIP_A (partition 8 in Yocto layout)
# We'll create a custom script that adds FIP partition after tfm_primary
set(create_flash_script ${tfm_source_dir}/platform/ext/target/arm/corstone1000/create-flash-image.sh)
set(fip_type_uuid "6823A0A1-6B6C-4E4A-A4E3-A4B9A5A60D23")

# FIP partitions sit right after their respective tfm_* partitions.
# TF-M layout (from create-flash-image.sh):
#   Bank 0: bl2_primary @ sector 72, tfm_primary @ sector 360 (+320K = ends at 1000)
#   Bank 1: bl2_secondary @ sector 32784, tfm_secondary @ sector 33072 (+320K = ends at 33712)
set(fip_a_start_sector 1000)
math(EXPR fip_b_start_sector "33072 - 360 + ${fip_a_start_sector}")

add_custom_command(
  OUTPUT ${CORSTONE1000_FIRMWARE_DIR}/cs1000_tfm.bin
  # First create base TF-M flash image
  COMMAND ${create_flash_script} ${tfm_binary_dir}/bin cs1000_tfm.bin
  COMMAND ${CMAKE_COMMAND} -E copy ${tfm_binary_dir}/bin/cs1000_tfm.bin ${CORSTONE1000_FIRMWARE_DIR}/cs1000_tfm.bin
  # Add FIP_A partition (after tfm_primary)
  COMMAND ${SGDISK} --new=11:${fip_a_start_sector}:+${SB_CONFIG_BOARD_CORSTONE1000_FIP_PARTITION_SIZE_MB}M --typecode=11:${fip_type_uuid} --change-name=11:'FIP_A' ${CORSTONE1000_FIRMWARE_DIR}/cs1000_tfm.bin || true
  # Add FIP_B partition (after tfm_secondary, same bank offset)
  COMMAND ${SGDISK} --new=12:${fip_b_start_sector}:+${SB_CONFIG_BOARD_CORSTONE1000_FIP_PARTITION_SIZE_MB}M --typecode=12:${fip_type_uuid} --change-name=12:'FIP_B' ${CORSTONE1000_FIRMWARE_DIR}/cs1000_tfm.bin || true
  DEPENDS tfm_secure_enclave
  COMMENT "Creating Secure Enclave flash image with FIP partitions"
)

add_custom_target(tfm_flash ALL DEPENDS ${CORSTONE1000_FIRMWARE_DIR}/cs1000_tfm.bin)

# =============================================================================
# TF-A Build for Host CPU (Cortex-A320)
# =============================================================================

message(STATUS "Corstone-1000: Building TF-A for Host CPU (Cortex-A320)")

# TF-A build type
if(SB_CONFIG_BOARD_CORSTONE1000_TFA_DEBUG)
  set(tfa_build_debug "1")
else()
  set(tfa_build_debug "0")
endif()

# Get the Zephyr binary from the main image
# Use the known sysbuild path structure: ${CMAKE_BINARY_DIR}/<image>/zephyr/zephyr.bin
set(zephyr_bl33 ${CMAKE_BINARY_DIR}/${DEFAULT_IMAGE}/zephyr/zephyr.bin)

# TF-A build arguments for Corstone-1000 with Cortex-A320
# Aligned with Yocto meta-arm CORSTONE1000-2025.12 reference build
set(tfa_plat "corstone1000")
set(tfa_extra_args
  TARGET_PLATFORM=fvp
  CORSTONE1000_CORTEX_A320=1
  HW_ASSISTED_COHERENCY=1
  USE_COHERENT_MEM=0
  CTX_INCLUDE_AARCH32_REGS=0
  NEED_BL32=no
  # PIE allows BL2 to run at any address (not just BL2_BASE)
  ENABLE_PIE=1
  # CPU resets directly into BL2 (no BL1 on host side)
  RESET_TO_BL2=1
  # A320 uses GICv3 (GIC-600), not GICv2
  FVP_USE_GIC_DRIVER=FVP_GICV3
  # ARMv9 feature enables for Cortex-A320
  ENABLE_FEAT_HCX=1
  ENABLE_FEAT_FGT=1
  ENABLE_FEAT_ECV=1
  ENABLE_FEAT_MTE2=1
  ENABLE_FEAT_AMU=1
  ENABLE_FEAT_CSV2_2=1
  ENABLE_SVE_FOR_NS=1
  ENABLE_SVE_FOR_SWD=1
  ENABLE_STACK_PROTECTOR=strong
  # GPT partition table support for FIP discovery in flash
  ARM_GPT_SUPPORT=1
  PSA_FWU_SUPPORT=1
  NR_OF_IMAGES_IN_FW_BANK=4
)

# Multi-core PSCI support for secondary CPU boot
if("${BOARD_QUALIFIERS}" MATCHES "/smp$")
  list(APPEND tfa_extra_args ENABLE_MULTICORE=1)
endif()

# TF-A requires an AArch64 toolchain (aarch64-zephyr-elf from the Zephyr SDK)
if(EXISTS "${ZEPHYR_SDK_INSTALL_DIR}/gnu/aarch64-zephyr-elf")
  set(tfa_cross_compile "${ZEPHYR_SDK_INSTALL_DIR}/gnu/aarch64-zephyr-elf/bin/aarch64-zephyr-elf-")
else()
  set(tfa_cross_compile "${ZEPHYR_SDK_INSTALL_DIR}/aarch64-zephyr-elf/bin/aarch64-zephyr-elf-")
endif()

# Find dtc (device tree compiler) - needed for TF-A DTS processing
find_program(DTC_EXECUTABLE dtc
  HINTS ${ZEPHYR_SDK_INSTALL_DIR}/hosttools/sysroots/x86_64-pokysdk-linux/usr/bin
        ${ZEPHYR_SDK_INSTALL_DIR}/sysroots/x86_64-pokysdk-linux/usr/bin)
if(DTC_EXECUTABLE)
  set(tfa_dtc_arg "DTC=${DTC_EXECUTABLE}")
else()
  message(WARNING "dtc not found. TF-A FIP generation may fail.")
  set(tfa_dtc_arg "")
endif()

# TF-A's upstream build system is GNU make.  It does not support ninja, so we
# always invoke 'make' here regardless of the generator chosen for Zephyr.
ExternalProject_Add(
  tfa_host
  SOURCE_DIR ${tfa_source_dir}
  BINARY_DIR ${tfa_binary_dir}
  CONFIGURE_COMMAND ""
  BUILD_COMMAND make -C ${tfa_source_dir}
    DEBUG=${tfa_build_debug}
    CROSS_COMPILE=${tfa_cross_compile}
    BUILD_BASE=${tfa_binary_dir}
    PLAT=${tfa_plat}
    BL33=${zephyr_bl33}
    ${tfa_dtc_arg}
    ${tfa_extra_args}
    all fip
  INSTALL_COMMAND ""
  BUILD_ALWAYS True
  USES_TERMINAL_BUILD True
)

# TF-A depends on Zephyr being built first
add_dependencies(tfa_host ${DEFAULT_IMAGE})

# TF-A output directory
if(SB_CONFIG_BOARD_CORSTONE1000_TFA_DEBUG)
  set(tfa_output_dir ${tfa_binary_dir}/${tfa_plat}/debug)
else()
  set(tfa_output_dir ${tfa_binary_dir}/${tfa_plat}/release)
endif()

# =============================================================================
# Sign TF-A BL2 with MCUboot-compatible format for TF-M verification
# =============================================================================
# TF-M Corstone1000 uses MCUBOOT_BUILTIN_KEY mode with EC-P256 signatures.
# Key mapping with BUILTIN_KEY (ecdsa.h bootutil_ecdsa_init increments key_id):
#   image_index -> key_id (in image_validate.c: key_id = image_index)
#   bootutil_ecdsa_init: key_id++ (to avoid PSA_KEY_ID_NULL=0)
#   So for Image 1: key_id = 1 + 1 = 2
#   PSA looks for key_id=2 -> desc_table[1] (key_id=idx+1=2)
#   desc_table[1] -> OTP KIND_1 (image_idx = key_id - 1 = 1)
# Therefore Image 1 (TF-A BL2) uses KIND_1 key (root-EC-P256_1.pem).
#
# Parameters from TF-M Corstone1000 config:
# - Header size: 0x1000 (4KB)
# - Load address: 0x623D1000 - BL2_SIGNATURE_BASE in SE address space
# - Signature: EC-P256 with ECDSA
# - Security counter: Required for MCUBOOT_HW_ROLLBACK_PROT

set(imgtool ${ZEPHYR_MCUBOOT_MODULE_DIR}/scripts/imgtool.py)
# The signing key ships as a static asset in the TF-M tree (bl2/ext/mcuboot/);
# it is the pre-shared root EC key matched against the OTP KIND_1 slot when
# verifying image 1 (TF-A BL2).  TF-M does not fetch any external mcuboot
# submodule.
set(signing_key ${tfm_source_dir}/bl2/ext/mcuboot/root-EC-P256_1.pem)

# TF-A BL2 slot size from flash_layout.h: SE_BL2_IMAGE_MAX_SIZE = 0x100000
set(tfa_slot_size 0x100000)
# Load address in SE address space (SE = host + 0x60000000).
# MCUboot copies the image (header + binary) to this address.
# Binary starts at load_addr + header_size = 0x62354000 (SE) = 0x02354000 (host).
# This matches the BIR address programmed in tfm_hal_multi_core.c.
# TF-A BL2 is compiled with ENABLE_PIE=1 so it runs at any address.
set(tfa_load_addr 0x62353000)
# Header size from TF-M config: BL2_HEADER_SIZE = 0x1000
set(tfa_header_size 0x1000)

add_custom_command(
  OUTPUT ${CORSTONE1000_FIRMWARE_DIR}/bl2_signed.bin
  COMMAND ${Python3_EXECUTABLE} ${imgtool} sign
    --key ${signing_key}
    --header-size ${tfa_header_size}
    --align 1
    --version 0.0.7
    --security-counter auto
    --pad-header
    --pad
    --slot-size ${tfa_slot_size}
    --load-addr ${tfa_load_addr}
    --boot-record NSPE
    ${tfa_output_dir}/bl2.bin
    ${CORSTONE1000_FIRMWARE_DIR}/bl2_signed.bin
  DEPENDS tfa_host
  COMMENT "Signing TF-A BL2 for MCUboot verification"
)

add_custom_target(tfa_bl2_signed ALL DEPENDS ${CORSTONE1000_FIRMWARE_DIR}/bl2_signed.bin)

# Rebuild FIP with signed BL2
# fiptool is built by TF-A's make into the build output directory
set(fiptool ${tfa_output_dir}/tools/fiptool/fiptool)

add_custom_command(
  OUTPUT ${CORSTONE1000_FIRMWARE_DIR}/fip.bin
  # Rebuild FIP with signed BL2, keeping other components from original TF-A build
  # FIP structure: BL2 (--tb-fw), BL31 (--soc-fw), BL33 (--nt-fw), TOS_FW_CONFIG
  COMMAND ${fiptool} create
    --tb-fw ${CORSTONE1000_FIRMWARE_DIR}/bl2_signed.bin
    --soc-fw ${tfa_output_dir}/bl31.bin
    --nt-fw ${zephyr_bl33}
    --tos-fw-config ${tfa_output_dir}/fdts/corstone1000_spmc_manifest.dtb
    ${CORSTONE1000_FIRMWARE_DIR}/fip.bin
  DEPENDS tfa_bl2_signed tfa_host ${DEFAULT_IMAGE}
    ${CORSTONE1000_FIRMWARE_DIR}/bl2_signed.bin ${zephyr_bl33}
  COMMENT "Creating FIP with signed TF-A BL2"
)

add_custom_target(tfa_fip ALL DEPENDS ${CORSTONE1000_FIRMWARE_DIR}/fip.bin)

# =============================================================================
# Combined Flash Image
# =============================================================================

# Write FIP to FIP_A partition in the flash image.
# TF-M expects FIP_SIGNATURE_AREA_SIZE (0x1000 = 4KB = 8 sectors) before FIP TOC header,
# so actual FIP data goes 8 sectors past the partition start.
math(EXPR fip_a_data_sector "${fip_a_start_sector} + 8")
math(EXPR fip_partition_bytes
  "${SB_CONFIG_BOARD_CORSTONE1000_FIP_PARTITION_SIZE_MB} * 1024 * 1024 - 4096")

# Generate a small check script at configure time
file(WRITE ${CORSTONE1000_FIRMWARE_DIR}/check_fip_size.sh
"#!/bin/sh
fip_size=$(stat -c%s \"$1\")
max_size=$2
if [ \"$fip_size\" -gt \"$max_size\" ]; then
  echo \"ERROR: FIP ($fip_size bytes) exceeds partition ($max_size bytes).\"
  echo \"Increase BOARD_CORSTONE1000_FIP_PARTITION_SIZE_MB in sysbuild Kconfig.\"
  exit 1
fi
")

add_custom_command(
  OUTPUT ${CORSTONE1000_FIRMWARE_DIR}/cs1000.bin
  COMMAND ${CMAKE_COMMAND} -E copy ${CORSTONE1000_FIRMWARE_DIR}/cs1000_tfm.bin ${CORSTONE1000_FIRMWARE_DIR}/cs1000.bin
  # Verify the FIP fits in the partition
  COMMAND sh ${CORSTONE1000_FIRMWARE_DIR}/check_fip_size.sh
    ${CORSTONE1000_FIRMWARE_DIR}/fip.bin ${fip_partition_bytes}
  COMMAND dd if=${CORSTONE1000_FIRMWARE_DIR}/fip.bin of=${CORSTONE1000_FIRMWARE_DIR}/cs1000.bin
    bs=512 seek=${fip_a_data_sector} conv=notrunc 2>/dev/null
  DEPENDS tfm_flash tfa_fip
    ${CORSTONE1000_FIRMWARE_DIR}/cs1000_tfm.bin ${CORSTONE1000_FIRMWARE_DIR}/fip.bin
  COMMENT "Creating combined flash image with TF-M and TF-A FIP"
)

add_custom_target(combined_flash ALL DEPENDS ${CORSTONE1000_FIRMWARE_DIR}/cs1000.bin)

message(STATUS "Corstone-1000 firmware will be in: ${CORSTONE1000_FIRMWARE_DIR}")
message(STATUS "  - bl1.bin: Secure Enclave ROM")
message(STATUS "  - cs1000.bin: Combined flash image with TF-M and TF-A FIP")

# =============================================================================
# Run target for 'west build -t run'
# =============================================================================
# The sub-build (board.cmake + armfvp.cmake) defines run_armfvp with all FVP
# flags. We expose a top-level 'run' target that ensures all firmware images
# are assembled before forwarding to the sub-build's run target.
add_custom_target(run
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/${DEFAULT_IMAGE} --target run
  DEPENDS combined_flash
  USES_TERMINAL
)
