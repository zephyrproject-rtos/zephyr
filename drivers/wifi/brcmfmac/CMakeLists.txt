# Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_WIFI_BRCMFMAC)

zephyr_library()
zephyr_library_sources(
  brcmfmac_core.c
  brcmfmac_chip.c
  brcmfmac_sdio.c
  brcmfmac_bcdc.c
  brcmfmac_net.c
  brcmfmac_fw_blob.c
)

set(brcmfmac_gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/brcmfmac)
zephyr_library_include_directories(${brcmfmac_gen_dir})

if(CONFIG_BUILD_ONLY_NO_BLOBS)
  # CI build path: hal_broadcom may not be present and blobs are not
  # fetched. Emit empty .inc files so brcmfmac_fw_blob.c compiles into
  # zero-length arrays; the driver will refuse to upload at runtime,
  # which is the expected failure mode for a no-blobs build.
  file(WRITE ${brcmfmac_gen_dir}/brcmfmac_fw.inc "")
  file(WRITE ${brcmfmac_gen_dir}/brcmfmac_nvram.inc "")
  file(WRITE ${brcmfmac_gen_dir}/brcmfmac_clm_blob.inc "")
else()
  # Pull the firmware and per-board NVRAM from hal_broadcom (fetched
  # via `west blobs fetch hal_broadcom`) and convert each into a
  # comma-separated byte sequence that brcmfmac_fw_blob.c #include's
  # as the body of a const array.
  zephyr_blobs_verify(MODULE hal_broadcom REQUIRED)
  set(brcmfmac_blob_dir
    ${ZEPHYR_HAL_BROADCOM_MODULE_DIR}/zephyr/blobs/img/brcmfmac)
  generate_inc_file_for_target(${ZEPHYR_CURRENT_LIBRARY}
    ${brcmfmac_blob_dir}/${CONFIG_WIFI_BRCMFMAC_FIRMWARE_FILE}
    ${brcmfmac_gen_dir}/brcmfmac_fw.inc)
  generate_inc_file_for_target(${ZEPHYR_CURRENT_LIBRARY}
    ${brcmfmac_blob_dir}/${CONFIG_WIFI_BRCMFMAC_NVRAM_FILE}
    ${brcmfmac_gen_dir}/brcmfmac_nvram.inc)

  # CLM blob is optional: a board that needs it sets
  # CONFIG_WIFI_BRCMFMAC_CLM_FILE in its defconfig; an unset/empty
  # string links a zero-length array and the driver skips upload.
  if(CONFIG_WIFI_BRCMFMAC_CLM_FILE STREQUAL "")
    file(WRITE ${brcmfmac_gen_dir}/brcmfmac_clm_blob.inc "")
  else()
    generate_inc_file_for_target(${ZEPHYR_CURRENT_LIBRARY}
      ${brcmfmac_blob_dir}/${CONFIG_WIFI_BRCMFMAC_CLM_FILE}
      ${brcmfmac_gen_dir}/brcmfmac_clm_blob.inc)
  endif()
endif()

endif()
