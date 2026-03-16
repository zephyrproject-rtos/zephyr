# Copyright (c) 2026 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

# west sign

if(CONFIG_BUILD_ONLY_NO_BLOBS OR CONFIG_SOC_AMEBA_NP_DISABLED)
  message(WARNING "
  ---------------------------------------------------------------------------------------
  Building only the Ameba SoC without binary blobs omits firmware for the non-Zephyr CPU.
  This is only meant for building images (CI) without running them on a real device.
  ---------------------------------------------------------------------------------------
  ")
elseif(CONFIG_SOC_AMEBA_NP_IMAGE)
  zephyr_blobs_verify(MODULE hal_realtek REQUIRED)

  set(hal_blob_dir ${ZEPHYR_HAL_REALTEK_MODULE_DIR}/zephyr/blobs/ameba/${CONFIG_SOC_SERIES}/bin/)

  if(CONFIG_SOC_SERIES_AMEBAD)
    set(blob_bin_file ${hal_blob_dir}/km0_image2_all.bin)
  elseif(CONFIG_SOC_SERIES_AMEBADPLUS)
    set(blob_bin_file ${hal_blob_dir}/km0_image2_all.bin)
  elseif(CONFIG_SOC_SERIES_AMEBAG2)
    set(blob_bin_file ${hal_blob_dir}/km4ns_image2_all.bin)
  endif()

  if(EXISTS ${blob_bin_file})
    set(python_script_args
      --soc ${CONFIG_SOC_SERIES}
      --bin-file ${ZEPHYR_BINARY_DIR}/${KERNEL_BIN_NAME}
      --out-dir ${CMAKE_BINARY_DIR}
      --module-dir ${ZEPHYR_HAL_REALTEK_MODULE_DIR}
    )

    add_custom_target(zephyr.raw.map ALL
      DEPENDS ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.raw.map
    )

    add_custom_command(
      OUTPUT ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.raw.map
      COMMAND ${CMAKE_NM} ${ZEPHYR_BINARY_DIR}/${KERNEL_ELF_NAME} | sort > ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.raw.map
      COMMAND ${CMAKE_OBJDUMP} -d ${ZEPHYR_BINARY_DIR}/${KERNEL_ELF_NAME} > ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.asm
      COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_HAL_REALTEK_MODULE_DIR}/ameba/scripts/merge_bin.py ${python_script_args}
      DEPENDS ${ZEPHYR_BINARY_DIR}/${KERNEL_ELF_NAME} ${blob_bin_file} ${ZEPHYR_HAL_REALTEK_MODULE_DIR}/ameba/${CONFIG_SOC_SERIES}/manifest.json5
    )
  endif()
endif()
