# SPDX-License-Identifier: Apache-2.0

if (CONFIG_SOF AND CONFIG_BOARD_IMX95_EVK_MIMX9596_M7_DDR)
  add_custom_target(zephyr.ri ALL
    DEPENDS ${CMAKE_BINARY_DIR}/zephyr/zephyr.ri
  )

  add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/zephyr/zephyr.ri
    COMMAND west sign --if-tool-available --tool rimage --build-dir ${CMAKE_BINARY_DIR} ${WEST_SIGN_OPTS}
    DEPENDS ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}
  )
endif()

if(CONFIG_BOARD_NXP_SPSDK_IMAGE OR (DEFINED ENV{CONFIG_BOARD_NXP_SPSDK_IMAGE}
  AND "$ENV{CONFIG_BOARD_NXP_SPSDK_IMAGE}" STREQUAL "y"))
  if(CONFIG_BOARD_IMX95_EVK_MIMX9596_M7)
    file(WRITE ${CMAKE_BINARY_DIR}/zephyr/imx95_evk_mimx9596_m7_ahab.yaml
      "
      family: mimx9596
      revision: a1
      target_memory: standard
      output:                   ${CMAKE_BINARY_DIR}/zephyr/flash.bin
      containers:
        - binary_container:
            path:               ${CMAKE_BINARY_DIR}/zephyr/imx95-19x19-lpddr5-evk-boot-firmware-0.1/mx95a0-ahab-container.img
        - container:
            srk_set: none
            images:
              - lpddr_imem:     ${CMAKE_BINARY_DIR}/zephyr/imx95-19x19-lpddr5-evk-boot-firmware-0.1/lpddr5_imem_v202311.bin
                lpddr_imem_qb:  ${CMAKE_BINARY_DIR}/zephyr/imx95-19x19-lpddr5-evk-boot-firmware-0.1/lpddr5_imem_qb_v202311.bin
                lpddr_dmem:     ${CMAKE_BINARY_DIR}/zephyr/imx95-19x19-lpddr5-evk-boot-firmware-0.1/lpddr5_dmem_v202311.bin
                lpddr_dmem_qb:  ${CMAKE_BINARY_DIR}/zephyr/imx95-19x19-lpddr5-evk-boot-firmware-0.1/lpddr5_dmem_qb_v202311.bin
                oei_ddr:        ${CMAKE_BINARY_DIR}/zephyr/imx95-19x19-lpddr5-evk-boot-firmware-0.1/oei-m33-ddr.bin
              - oei_tcm:        ${CMAKE_BINARY_DIR}/zephyr/imx95-19x19-lpddr5-evk-boot-firmware-0.1/oei-m33-tcm.bin
              - system_manager: ${CMAKE_BINARY_DIR}/zephyr/imx95-19x19-lpddr5-evk-boot-firmware-0.1/m33_image-mx95alt.bin
              - cortex_m7_app:  ${CMAKE_BINARY_DIR}/zephyr/zephyr.bin
              - v2x_dummy: true
      ")
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND chmod +x ${ZEPHYR_HAL_NXP_MODULE_DIR}/zephyr/blobs/imx-boot-firmware/imx95-19x19-lpddr5-evk-boot-firmware-0.1.bin
      COMMAND ${ZEPHYR_HAL_NXP_MODULE_DIR}/zephyr/blobs/imx-boot-firmware/imx95-19x19-lpddr5-evk-boot-firmware-0.1.bin --auto-accept --force
      COMMAND nxpimage ahab export -c ${CMAKE_BINARY_DIR}/zephyr/imx95_evk_mimx9596_m7_ahab.yaml
    )
  endif()
  zephyr_blobs_verify(FILES
    ${ZEPHYR_HAL_NXP_MODULE_DIR}/zephyr/blobs/imx-boot-firmware/imx95-19x19-lpddr5-evk-boot-firmware-0.1.bin
    REQUIRED)
endif()
