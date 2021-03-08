# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(${ZEPHYR_BASE}/subsys/net/l2)

if(CONFIG_WIFI_WFX200)
  zephyr_include_directories(./)

  set(hal_dir               ${ZEPHYR_SILABS_MODULE_DIR})
  set(hal_blobs_dir         ${ZEPHYR_SILABS_MODULE_DIR}/zephyr/blobs)
  set(blob_gen_dir          ${ZEPHYR_BINARY_DIR}/include/generated)
  set(blob_gen_inc_file     ${blob_gen_dir}/wfm_wf200_C0.sec.inc)
  set(blob_bin_file         ${hal_blobs_dir}/wfx_fmac_driver/wfm_wf200_C0.sec)

  zephyr_linker_sources(RODATA SORT_KEY 0 wfx200_firmware.ld)
  if(EXISTS ${blob_bin_file})
    generate_inc_file_for_target(app ${blob_bin_file} ${blob_gen_inc_file})
    zephyr_library_sources(wfx200_firmware.c)
  else()
    message(WARNING "Can't find wf200 firmware, please execute: \'west blobs fetch silabs\'")
  endif()

  zephyr_library_sources(
    wfx200.c
    wfx200_host.c
    wfx200_events.c
  )
endif()
