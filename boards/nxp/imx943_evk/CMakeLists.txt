# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_NXP_SPSDK_IMAGE OR (DEFINED ENV{USE_NXP_SPSDK_IMAGE}
  AND "$ENV{USE_NXP_SPSDK_IMAGE}" STREQUAL "y"))
  find_program(7Z_EXECUTABLE 7z REQUIRED)
  set(FIRMWARE_RELEASE "imx943-evk-boot-firmware-6.12.34-2.1.0")
  # Parse SPSDK version
  execute_process(
    COMMAND spsdk --version
    OUTPUT_VARIABLE SPSDK_VERSION_OUTPUT
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" SPSDK_VERSION "${SPSDK_VERSION_OUTPUT}")
  message(STATUS "SPSDK version is ${SPSDK_VERSION}")

  if(CONFIG_BOARD_NXP_LPDDR4)
    set(DDR_FW "lpddr4x")
  else()
    set(DDR_FW "lpddr5")
  endif()

  if(CONFIG_SOC_MIMX94398_M7_0)
    set(AHAB_CONFIG_FILE "imx943_evk_mimx94398_m7_0_ahab.yaml")

    file(WRITE ${CMAKE_BINARY_DIR}/zephyr/${AHAB_CONFIG_FILE}
      "
      family: mimx943
      revision: a0
      target_memory: standard
      output:                   ${CMAKE_BINARY_DIR}/zephyr/flash.bin
      containers:
        - binary_container:
            path:               ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/mx943a0-ahab-container.img
        - container:
            srk_set: none
            images:
              - lpddr_imem:     ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/${DDR_FW}_imem_v202409.bin
                lpddr_imem_qb:  ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/${DDR_FW}_imem_qb_v202409.bin
                lpddr_dmem:     ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/${DDR_FW}_dmem_v202409.bin
                lpddr_dmem_qb:  ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/${DDR_FW}_dmem_qb_v202409.bin
                oei_ddr:        ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/oei-m33-ddr-${DDR_FW}.bin
              - system_manager: ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/m33_image-mx94alt.bin
              - cortex_m7_app:  ${CMAKE_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.bin
    ")
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND ${7Z_EXECUTABLE} x ${ZEPHYR_HAL_NXP_MODULE_DIR}/zephyr/blobs/imx-boot-firmware/${FIRMWARE_RELEASE}.bin -o./imx-boot-firmware -aos -bso0 -bse1
      COMMAND ${7Z_EXECUTABLE} x imx-boot-firmware/${FIRMWARE_RELEASE} -aos -bso0 -bse1
      COMMAND nxpimage ahab export -c ${CMAKE_BINARY_DIR}/zephyr/${AHAB_CONFIG_FILE}
    )
  elseif(CONFIG_SOC_MIMX94398_M7_1)
    set(AHAB_CONFIG_FILE "imx943_evk_mimx94398_m7_1_ahab.yaml")

    file(WRITE ${CMAKE_BINARY_DIR}/zephyr/${AHAB_CONFIG_FILE}
      "
      family: mimx943
      revision: a0
      target_memory: standard
      output:                   ${CMAKE_BINARY_DIR}/zephyr/flash.bin
      containers:
        - binary_container:
            path:               ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/mx943a0-ahab-container.img
        - container:
            srk_set: none
            images:
              - lpddr_imem:     ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/${DDR_FW}_imem_v202409.bin
                lpddr_imem_qb:  ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/${DDR_FW}_imem_qb_v202409.bin
                lpddr_dmem:     ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/${DDR_FW}_dmem_v202409.bin
                lpddr_dmem_qb:  ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/${DDR_FW}_dmem_qb_v202409.bin
                oei_ddr:        ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/oei-m33-ddr-${DDR_FW}.bin
              - system_manager: ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/m33_image-mx94alt.bin
              - cortex_m7_2_app:  ${CMAKE_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.bin
    ")
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND ${7Z_EXECUTABLE} x ${ZEPHYR_HAL_NXP_MODULE_DIR}/zephyr/blobs/imx-boot-firmware/${FIRMWARE_RELEASE}.bin -o./imx-boot-firmware -aos -bso0 -bse1
      COMMAND ${7Z_EXECUTABLE} x imx-boot-firmware/${FIRMWARE_RELEASE} -aos -bso0 -bse1
      COMMAND nxpimage ahab export -c ${CMAKE_BINARY_DIR}/zephyr/${AHAB_CONFIG_FILE}
    )
  elseif(CONFIG_SOC_MIMX94398_M33)
    set(AHAB_CONFIG_FILE "imx943_evk_mimx94398_m33_ahab.yaml")

    file(WRITE ${CMAKE_BINARY_DIR}/zephyr/${AHAB_CONFIG_FILE}
      "
      family: mimx943
      revision: a0
      target_memory: standard
      output:                   ${CMAKE_BINARY_DIR}/zephyr/flash.bin
      containers:
        - binary_container:
            path:               ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/mx943a0-ahab-container.img
        - container:
            srk_set: none
            images:
              - lpddr_imem:     ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/${DDR_FW}_imem_v202409.bin
                lpddr_imem_qb:  ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/${DDR_FW}_imem_qb_v202409.bin
                lpddr_dmem:     ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/${DDR_FW}_dmem_v202409.bin
                lpddr_dmem_qb:  ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/${DDR_FW}_dmem_qb_v202409.bin
                oei_ddr:        ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/oei-m33-ddr-${DDR_FW}.bin
              - system_manager: ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/m33_image-mx94alt.bin
              - cortex_m33_2_app:  ${CMAKE_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.bin
    ")
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND ${7Z_EXECUTABLE} x ${ZEPHYR_HAL_NXP_MODULE_DIR}/zephyr/blobs/imx-boot-firmware/${FIRMWARE_RELEASE}.bin -o./imx-boot-firmware -aos -bso0 -bse1
      COMMAND ${7Z_EXECUTABLE} x imx-boot-firmware/${FIRMWARE_RELEASE} -aos -bso0 -bse1
      COMMAND nxpimage ahab export -c ${CMAKE_BINARY_DIR}/zephyr/${AHAB_CONFIG_FILE}
    )
  elseif(CONFIG_SOC_MIMX94398_A55)
    file(WRITE ${CMAKE_BINARY_DIR}/zephyr/imx943_evk_mimx94398_a55_ahab_primary.yaml
      "
      family: mimx943
      revision: a0
      target_memory: standard
      output:                   ${CMAKE_BINARY_DIR}/zephyr/primary_ahab.bin
      containers:
        - binary_container:
            path:               ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/mx943a0-ahab-container.img
        - container:
            srk_set: none
            images:
              - lpddr_imem:     ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/${DDR_FW}_imem_v202409.bin
                lpddr_imem_qb:  ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/${DDR_FW}_imem_qb_v202409.bin
                lpddr_dmem:     ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/${DDR_FW}_dmem_v202409.bin
                lpddr_dmem_qb:  ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/${DDR_FW}_dmem_qb_v202409.bin
                oei_ddr:        ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/oei-m33-ddr-${DDR_FW}.bin
              - system_manager: ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/m33_image-mx94evk.bin
              - spl:            ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/u-boot-spl.bin-imx943-19x19-lpddr5-evk-sd
    ")
    file(WRITE ${CMAKE_BINARY_DIR}/zephyr/imx943_evk_mimx94398_a55_ahab_secondary.yaml
      "
      family: mimx943
      revision: a0
      target_memory: standard
      output:                   ${CMAKE_BINARY_DIR}/zephyr/secondary_ahab.bin
      containers:
        - container:
            srk_set: none
            images:
              - atf:            ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/bl31-imx943-zephyr.bin
              - image_path:     ${CMAKE_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.bin
                load_address:   '${CONFIG_SRAM_BASE_ADDRESS}'
                entry_point:    '${CONFIG_SRAM_BASE_ADDRESS}'
                image_type:     executable
                core_id:        cortex-a55
                is_encrypted:   false
    ")
    file(WRITE ${CMAKE_BINARY_DIR}/zephyr/imx943_evk_mimx94398_a55_ahab_flash_template.yaml
      "
      family: mimx943
      revision: latest
      memory_type: serial_downloader
      primary_image_container_set: ${CMAKE_BINARY_DIR}/zephyr/imx943_evk_mimx94398_a55_ahab_primary.yaml
      secondary_image_container_set: ${CMAKE_BINARY_DIR}/zephyr/imx943_evk_mimx94398_a55_ahab_secondary.yaml
   ")
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND ${7Z_EXECUTABLE} x ${ZEPHYR_HAL_NXP_MODULE_DIR}/zephyr/blobs/imx-boot-firmware/${FIRMWARE_RELEASE}.bin -o./imx-boot-firmware -aos -bso0 -bse1
      COMMAND ${7Z_EXECUTABLE} x imx-boot-firmware/${FIRMWARE_RELEASE} -aos -bso0 -bse1
    )
    if(SPSDK_VERSION VERSION_GREATER_EQUAL "3.0.0")
      set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
        COMMAND nxpimage bootable-image export -c ${CMAKE_BINARY_DIR}/zephyr/imx943_evk_mimx94398_a55_ahab_flash_template.yaml -o flash.bin
      )
    else()
      set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
        COMMAND nxpimage bootable-image merge -c ${CMAKE_BINARY_DIR}/zephyr/imx943_evk_mimx94398_a55_ahab_flash_template.yaml -o flash.bin
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
