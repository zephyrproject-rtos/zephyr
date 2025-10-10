#-------------------------------------------------------------------------------
# Copyright (c) 2020-2023, Arm Limited. All rights reserved.
# Copyright (c) 2021 STMicroelectronics. All rights reserved.
# Copyright (c) 2022 Cypress Semiconductor Corporation (an Infineon company)
# or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

################################## BL2 #########################################################################################################
set(MCUBOOT_IMAGE_NUMBER                   2           CACHE STRING    "Whether to combine S and NS into either 1 image, or sign each separately")
set(BL2_TRAILER_SIZE                       0x9000      CACHE STRING    "Trailer size")
set(MCUBOOT_ALIGN_VAL                      16          CACHE STRING    "Align option to build image with imgtool")
set(MCUBOOT_UPGRADE_STRATEGY      "SWAP_USING_SCRATCH" CACHE STRING    "Upgrade strategy for images")
set(TFM_PARTITION_PLATFORM                 ON          CACHE BOOL      "Enable platform partition")
set(MCUBOOT_CONFIRM_IMAGE                  ON          CACHE BOOL      "Whether to confirm the image if REVERT is supported in MCUboot")
set(MCUBOOT_BOOTSTRAP                      ON          CACHE BOOL      "Allow initial state with images in secondary slots(empty primary slots)")
set(MCUBOOT_ENC_IMAGES                     ON          CACHE BOOL      "Enable encrypted image upgrade support")
set(MCUBOOT_ENCRYPT_RSA                    ON          CACHE BOOL      "Use RSA for encrypted image upgrade support")
set(MCUBOOT_DATA_SHARING                   ON          CACHE BOOL      "Enable Data Sharing")
cmake_path(NORMAL_PATH MCUBOOT_KEY_S)
cmake_path(NORMAL_PATH MCUBOOT_KEY_NS)
cmake_path(GET MCUBOOT_KEY_S PARENT_PATH MCUBOOT_KEY_PATH)
set(MCUBOOT_KEY_ENC "${MCUBOOT_KEY_PATH}/rsa-2048-public-bl2.pem" CACHE FILEPATH "Path to key with which to encrypt binary")

################################## Dependencies ################################################################################################
set(TFM_PARTITION_INTERNAL_TRUSTED_STORAGE ON          CACHE BOOL      "Enable Internal Trusted Storage partition")
set(TFM_PARTITION_CRYPTO                   ON          CACHE BOOL      "Enable Crypto partition")
set(CRYPTO_HW_ACCELERATOR                  ON          CACHE BOOL      "Whether to enable the crypto hardware accelerator on supported platforms")
set(MBEDCRYPTO_BUILD_TYPE                  minsizerel  CACHE STRING    "Build type of Mbed Crypto library")
set(TFM_DUMMY_PROVISIONING                 OFF         CACHE BOOL      "Provision with dummy values. NOT to be used in production")
set(PLATFORM_DEFAULT_OTP_WRITEABLE         OFF         CACHE BOOL      "Use on chip flash with write support")
set(PLATFORM_DEFAULT_NV_COUNTERS           OFF         CACHE BOOL      "Use default nv counter implementation.")
set(PS_CRYPTO_AEAD_ALG                     PSA_ALG_GCM CACHE STRING    "The AEAD algorithm to use for authenticated encryption in Protected Storage")
set(MCUBOOT_FIH_PROFILE                    LOW         CACHE STRING    "Fault injection hardening profile [OFF, LOW, MEDIUM, HIGH]")

################################## Platform-specific configurations ############################################################################
set(CONFIG_TFM_USE_TRUSTZONE               ON          CACHE BOOL      "Use TrustZone")
set(TFM_MULTI_CORE_TOPOLOGY                OFF         CACHE BOOL      "Platform has multi core")
set(PLATFORM_HAS_FIRMWARE_UPDATE_SUPPORT   ON          CACHE BOOL      "Whether the platform has firmware update support")
set(STSAFEA                                OFF         CACHE BOOL      "Activate ST SAFE SUPPORT")

################################## FIRMWARE_UPDATE #############################################################################################
set(TFM_PARTITION_FIRMWARE_UPDATE          ON          CACHE BOOL      "Enable firmware update partition")
set(MCUBOOT_HW_ROLLBACK_PROT               ON          CACHE BOOL      "Security counter validation against non-volatile HW counters")
set(TFM_FWU_BOOTLOADER_LIB                 "mcuboot"   CACHE STRING    "Bootloader configure file for Firmware Update partition")
set(TFM_CONFIG_FWU_MAX_WRITE_SIZE          8192        CACHE STRING    "The maximum permitted size for block in psa_fwu_write, in bytes.")
set(TFM_CONFIG_FWU_MAX_MANIFEST_SIZE       0           CACHE STRING    "The maximum permitted size for manifest in psa_fwu_start(), in bytes.")
set(FWU_DEVICE_CONFIG_FILE                 ""          CACHE STRING    "The device configuration file for Firmware Update partition")
set(FWU_SUPPORT_TRIAL_STATE                ON          CACHE BOOL      "Device support TRIAL component state.")
set(DMCUBOOT_UPGRADE_STRATEGY              SWAP_USING_MOVE)
set(DEFAULT_MCUBOOT_FLASH_MAP              ON          CACHE BOOL      "Whether to use the default flash map defined by TF-M project")
