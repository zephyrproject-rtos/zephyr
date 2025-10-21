# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_NXP_SPSDK_IMAGE OR (DEFINED ENV{USE_NXP_SPSDK_IMAGE}
  AND "$ENV{USE_NXP_SPSDK_IMAGE}" STREQUAL "y"))
  find_program(7Z_EXECUTABLE 7z REQUIRED)
  set(FIRMWARE_RELEASE "imx95-19x19-lpddr5-evk-boot-firmware-0.1")
  # Parse SPSDK version
  execute_process(
    COMMAND spsdk --version
    OUTPUT_VARIABLE SPSDK_VERSION_OUTPUT
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" SPSDK_VERSION "${SPSDK_VERSION_OUTPUT}")
  message(STATUS "SPSDK version is ${SPSDK_VERSION}")

  if(CONFIG_BOARD_IMX95_EVK_MIMX9596_M7)
    set(AHAB_CONFIG_FILE "imx95_evk_mimx9596_m7_ahab.yaml")

    file(WRITE ${CMAKE_BINARY_DIR}/zephyr/${AHAB_CONFIG_FILE}
      "
      family: mimx9596
      revision: a1
      target_memory: standard
      output:                   ${CMAKE_BINARY_DIR}/zephyr/flash.bin
      containers:
        - binary_container:
            path:               ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/mx95a0-ahab-container.img
        - container:
            srk_set: none
            images:
              - lpddr_imem:     ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/lpddr5_imem_v202311.bin
                lpddr_imem_qb:  ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/lpddr5_imem_qb_v202311.bin
                lpddr_dmem:     ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/lpddr5_dmem_v202311.bin
                lpddr_dmem_qb:  ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/lpddr5_dmem_qb_v202311.bin
                oei_ddr:        ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/oei-m33-ddr.bin
              - oei_tcm:        ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/oei-m33-tcm.bin
              - system_manager: ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/m33_image-mx95alt.bin
              - cortex_m7_app:  ${CMAKE_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.bin
              - v2x_dummy: true
      ")
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND ${7Z_EXECUTABLE} x ${ZEPHYR_HAL_NXP_MODULE_DIR}/zephyr/blobs/imx-boot-firmware/${FIRMWARE_RELEASE}.bin -o./imx-boot-firmware -aos -bso0 -bse1
      COMMAND ${7Z_EXECUTABLE} x imx-boot-firmware/${FIRMWARE_RELEASE} -aos -bso0 -bse1
      COMMAND nxpimage ahab export -c ${CMAKE_BINARY_DIR}/zephyr/${AHAB_CONFIG_FILE}
    )
  elseif(CONFIG_SOC_MIMX9596_A55)
    file(WRITE ${CMAKE_BINARY_DIR}/zephyr/imx95_evk_mimx9596_a55_ahab_primary.yaml
      "
      family: mimx9596
      revision: a1
      target_memory: standard
      output:                   ${CMAKE_BINARY_DIR}/zephyr/primary_ahab.bin
      containers:
        - binary_container:
            path:               ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/mx95a0-ahab-container.img
        - container:
            srk_set: none
            images:
              - lpddr_imem:     ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/lpddr5_imem_v202311.bin
                lpddr_imem_qb:  ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/lpddr5_imem_qb_v202311.bin
                lpddr_dmem:     ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/lpddr5_dmem_v202311.bin
                lpddr_dmem_qb:  ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/lpddr5_dmem_qb_v202311.bin
                oei_ddr:        ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/oei-m33-ddr.bin
              - oei_tcm:        ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/oei-m33-tcm.bin
              - system_manager: ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/m33_image-mx95evk.bin
              - spl:            ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/u-boot-spl.bin-imx95-19x19-lpddr5-evk-sd
              - v2x_dummy: true
    ")
    file(WRITE ${CMAKE_BINARY_DIR}/zephyr/imx95_evk_mimx9596_a55_ahab_secondary.yaml
      "
      family: mimx9596
      revision: a1
      target_memory: standard
      output:                   ${CMAKE_BINARY_DIR}/zephyr/secondary_ahab.bin
      containers:
        - container:
            srk_set: none
            images:
              - atf:            ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/bl31-imx95-zephyr.bin
              - image_path:     ${CMAKE_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.bin
                load_address:   '${CONFIG_SRAM_BASE_ADDRESS}'
                entry_point:    '${CONFIG_SRAM_BASE_ADDRESS}'
                image_type:     executable
                core_id:        cortex-a55
                is_encrypted:   false
    ")
    file(WRITE ${CMAKE_BINARY_DIR}/zephyr/imx95_evk_mimx9596_a55_ahab_flash_template.yaml
      "
      family: mimx9596
      revision: latest
      memory_type: serial_downloader
      primary_image_container_set: ${CMAKE_BINARY_DIR}/zephyr/imx95_evk_mimx9596_a55_ahab_primary.yaml
      secondary_image_container_set: ${CMAKE_BINARY_DIR}/zephyr/imx95_evk_mimx9596_a55_ahab_secondary.yaml
   ")
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND ${7Z_EXECUTABLE} x ${ZEPHYR_HAL_NXP_MODULE_DIR}/zephyr/blobs/imx-boot-firmware/${FIRMWARE_RELEASE}.bin -o./imx-boot-firmware -aos -bso0 -bse1
      COMMAND ${7Z_EXECUTABLE} x imx-boot-firmware/${FIRMWARE_RELEASE} -aos -bso0 -bse1
    )
    if(SPSDK_VERSION VERSION_GREATER_EQUAL "3.0.0")
      set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
        COMMAND nxpimage bootable-image export -c ${CMAKE_BINARY_DIR}/zephyr/imx95_evk_mimx9596_a55_ahab_flash_template.yaml -o flash.bin
      )
    else()
      set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
        COMMAND nxpimage bootable-image merge -c ${CMAKE_BINARY_DIR}/zephyr/imx95_evk_mimx9596_a55_ahab_flash_template.yaml -o flash.bin
      )
    endif()
  else()
    message(FATAL_ERROR "SPSDK Image not supported on the platform!")
  endif()
  zephyr_blobs_verify(FILES
    ${ZEPHYR_HAL_NXP_MODULE_DIR}/zephyr/blobs/imx-boot-firmware/${FIRMWARE_RELEASE}.bin
    REQUIRED)
endif()

zephyr_library()
zephyr_library_sources(board.c)
