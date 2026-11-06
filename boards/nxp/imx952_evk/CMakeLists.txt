# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_NXP_SPSDK_IMAGE OR (DEFINED ENV{USE_NXP_SPSDK_IMAGE}
  AND "$ENV{USE_NXP_SPSDK_IMAGE}" STREQUAL "y"))
  find_program(7Z_EXECUTABLE 7z REQUIRED)
  set(FIRMWARE_RELEASE "imx952-evk-boot-firmware-6.18.20-2.0.0")
  # Parse SPSDK version
  execute_process(
    COMMAND spsdk --version
    OUTPUT_VARIABLE SPSDK_VERSION_OUTPUT
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" SPSDK_VERSION "${SPSDK_VERSION_OUTPUT}")
  message(STATUS "SPSDK version is ${SPSDK_VERSION}")
  if(SPSDK_VERSION VERSION_LESS "3.7.0")
    message(FATAL_ERROR "SPSDK version ${SPSDK_VERSION} is not supported. Minimum required version is 3.7.0")
  endif()

  if(CONFIG_BOARD_IMX952_EVK_MIMX9529_M7 OR CONFIG_BOARD_IMX952_EVK_MIMX9529_M7_DDR)
    set(AHAB_CONFIG_FILE "imx952_evk_mimx9529_m7_ahab.yaml")

    file(WRITE ${CMAKE_BINARY_DIR}/zephyr/${AHAB_CONFIG_FILE}
      "
      family: mx952
      revision: a0
      target_memory: standard
      output:                   ${CMAKE_BINARY_DIR}/zephyr/flash.bin
      containers:
        - binary_container:
            path:               ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/mx952a0-ahab-container.img
        - container:
            srk_set: none
            images:
              - lpddr_imem:     ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/lpddr5_imem_v202409.bin
                lpddr_imem_qb:  ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/lpddr5_imem_qb_v202409.bin
                lpddr_dmem:     ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/lpddr5_dmem_v202409.bin
                lpddr_dmem_qb:  ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/lpddr5_dmem_qb_v202409.bin
                oei_ddr:        ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/oei-m33-ddr.bin
              - system_manager: ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/m33_image-mx952alt.bin
              - cortex_m7_app:  ${CMAKE_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.bin
              - v2x_dummy: true
      ")
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND ${7Z_EXECUTABLE} x ${ZEPHYR_HAL_NXP_MODULE_DIR}/zephyr/blobs/imx-boot-firmware/${FIRMWARE_RELEASE}.bin -o./imx-boot-firmware -aos -bso0 -bse1
      COMMAND ${7Z_EXECUTABLE} x imx-boot-firmware/${FIRMWARE_RELEASE} -aos -bso0 -bse1
      COMMAND nxpimage ahab export -c ${CMAKE_BINARY_DIR}/zephyr/${AHAB_CONFIG_FILE}
    )
  elseif(CONFIG_SOC_MIMX9529_A55)
    if(CONFIG_SRAM_DEPRECATED_KCONFIG_SET)
      set(RAM_ADDR ${CONFIG_SRAM_BASE_ADDRESS})
    else()
      dt_chosen(chosen_sram_path PROPERTY "zephyr,sram")
      dt_reg_addr(RAM_ADDR PATH "${chosen_sram_path}")
    endif()

    file(WRITE ${CMAKE_BINARY_DIR}/zephyr/imx952_evk_mimx9529_a55_ahab_primary.yaml
      "
      family: mimx95294
      revision: latest
      target_memory: standard
      output:                   ${CMAKE_BINARY_DIR}/zephyr/primary_ahab.bin
      containers:
        - binary_container:
            path:               ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/mx952a0-ahab-container.img
        - container:
            srk_set: none
            fuse_version: 0
            sw_version: 0
            images:
              - lpddr_imem:     ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/lpddr5_imem_v202409.bin
                lpddr_imem_qb:  ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/lpddr5_imem_qb_v202409.bin
                lpddr_dmem:     ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/lpddr5_dmem_v202409.bin
                lpddr_dmem_qb:  ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/lpddr5_dmem_qb_v202409.bin
                oei_ddr:        ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/oei-m33-ddr.bin
              - system_manager: ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/m33_image-mx952rte.bin
              - v2x_dummy: true
              - spl:            ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/u-boot-spl.bin-imx952-19x19-lpddr5-evk-sd
    ")
    file(WRITE ${CMAKE_BINARY_DIR}/zephyr/imx952_evk_mimx9529_a55_ahab_secondary.yaml
      "
      family: mimx95294
      revision: latest
      target_memory: standard
      output:                   ${CMAKE_BINARY_DIR}/zephyr/secondary_ahab.bin
      containers:
        - container:
            srk_set: none
            fuse_version: 0
            sw_version: 0
            images:
              - atf:            ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/bl31-imx952-zephyr.bin
              - uboot:          ${CMAKE_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.bin
                load_address:   '${RAM_ADDR}'
                entry_point:    '${RAM_ADDR}'
                image_type:     executable
                core_id:        cortex-a55
                is_encrypted:   false
    ")
    file(WRITE ${CMAKE_BINARY_DIR}/zephyr/imx952_evk_mimx9529_a55_ahab_flash.yaml
      "
      family: mimx95294
      revision: latest
      memory_type: serial_downloader
      output: flash.bin
      output_format: bin
      init_offset: 0
      post_export: output
      spl: ${CMAKE_BINARY_DIR}/zephyr/imx952_evk_mimx9529_a55_ahab_primary.yaml
      uboot: ${CMAKE_BINARY_DIR}/zephyr/imx952_evk_mimx9529_a55_ahab_secondary.yaml
   ")
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND ${7Z_EXECUTABLE} x ${ZEPHYR_HAL_NXP_MODULE_DIR}/zephyr/blobs/imx-boot-firmware/${FIRMWARE_RELEASE}.bin -o./imx-boot-firmware -aos -bso0 -bse1
      COMMAND ${7Z_EXECUTABLE} x imx-boot-firmware/${FIRMWARE_RELEASE} -aos -bso0 -bse1
    )
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND nxpimage bootable-image export -c ${CMAKE_BINARY_DIR}/zephyr/imx952_evk_mimx9529_a55_ahab_flash.yaml
    )
  else()
    message(FATAL_ERROR "SPSDK Image not supported on the platform!")
  endif()
  zephyr_blobs_verify(FILES
    ${ZEPHYR_HAL_NXP_MODULE_DIR}/zephyr/blobs/imx-boot-firmware/${FIRMWARE_RELEASE}.bin
    REQUIRED)
endif()

zephyr_library()
zephyr_library_sources(board.c)
