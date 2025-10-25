# Copyright (c) 2024 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(${ZEPHYR_BASE}/drivers)
zephyr_include_directories(.)

zephyr_sources(soc.c)

zephyr_linker_sources(SECTIONS boot_section.ld)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")

set(PYTHON_SCRIPT_ARGS
  --soc ${CONFIG_SOC_SERIES}
  --bin-file ${ZEPHYR_BINARY_DIR}/${KERNEL_BIN_NAME}
  --out-dir ${CMAKE_BINARY_DIR}
  --module-dir ${ZEPHYR_HAL_REALTEK_MODULE_DIR}
)

#  west sign

file(GLOB BIN_FILES "${ZEPHYR_HAL_REALTEK_MODULE_DIR}/zephyr/blobs/${CONFIG_SOC_SERIES}/bin/*.bin")

if(BIN_FILES)
  add_custom_target(zephyr.raw.map ALL
    DEPENDS ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.raw.map
  )

  add_custom_command(
    OUTPUT ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.raw.map
    COMMAND ${CMAKE_NM} ${ZEPHYR_BINARY_DIR}/${KERNEL_ELF_NAME} | sort > ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.raw.map
    COMMAND ${CMAKE_OBJDUMP} -d ${ZEPHYR_BINARY_DIR}/${KERNEL_ELF_NAME} > ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.asm
    COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_HAL_REALTEK_MODULE_DIR}/ameba/scripts/merge_bin.py ${PYTHON_SCRIPT_ARGS}
    DEPENDS ${ZEPHYR_BINARY_DIR}/${KERNEL_ELF_NAME} ${ZEPHYR_HAL_REALTEK_MODULE_DIR}/ameba/${CONFIG_SOC_SERIES}/manifest.json5
  )
else()
  message(WARNING "Missing Realtek blobs. Run: `west blobs fetch hal_realtek`")
endif()
