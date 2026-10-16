# Copyright (c) 2026 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

# The vendor WiFi driver headers redefine CONFIG_LITTLE_ENDIAN; a system
# include silences their diagnostics. ameba-rtos has no zephyr/module.yml, so
# reach it via WEST_TOPDIR.
set(ameba_rtos_wifi_driver_inc
    ${WEST_TOPDIR}/modules/hal/ameba-rtos/component/wifi/driver/include)
if(EXISTS ${ameba_rtos_wifi_driver_inc})
  zephyr_system_include_directories(${ameba_rtos_wifi_driver_inc})
endif()

# ---------------------------------------------------------------------------
# Linker script and SoC sources: branch per CPU cluster.
#   CA32 (Cortex-A32) : SMP application domain, loaded by TF-A
#   KM4  (Cortex-M55) : KM4 application (image2)
#   KM0  (Cortex-M23) : low-power core application
# ---------------------------------------------------------------------------

if(CONFIG_SOC_RTL8730E_KM4)
  # KM4 Cortex-M55: non-secure image2.
  include(${CMAKE_CURRENT_LIST_DIR}/km4/CMakeLists.txt)

elseif(CONFIG_SOC_RTL8730E_KM0)
  # KM0 Cortex-M23: low-power core.
  include(${CMAKE_CURRENT_LIST_DIR}/km0/CMakeLists.txt)

else()
  # CA32 Cortex-A32: Zephyr application domain
  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_a_r/scripts/linker.ld CACHE INTERNAL "")

  # The HAL blobs need the armv7-a+neon-fp16 hard-float multilib, but
  # -mfpu=neon-fp-armv8 matches none in asdk-10.3.1 and newlib falls back to
  # soft-float. Resolve libgcc/libc with hard-float flags and swap newlib's -L.
  execute_process(
    COMMAND ${CMAKE_C_COMPILER} -march=armv7-a+neon-fp16 -mfloat-abi=hard -print-libgcc-file-name
    OUTPUT_VARIABLE _libgcc_file
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  get_filename_component(_libgcc_dir ${_libgcc_file} DIRECTORY)
  zephyr_link_libraries(-L${_libgcc_dir})

  if(LIBC_LIBRARY_DIR AND SYSROOT_DIR)
    execute_process(
      COMMAND ${CMAKE_C_COMPILER} -march=armv7-a+neon-fp16 -mfloat-abi=hard -print-multi-directory
      OUTPUT_VARIABLE _hf_multilib_subdir
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(_hf_libc_dir "${SYSROOT_DIR}/lib/${_hf_multilib_subdir}")
    if(EXISTS "${_hf_libc_dir}")
      get_property(_iface_libs TARGET zephyr_interface PROPERTY INTERFACE_LINK_LIBRARIES)
      list(FIND _iface_libs "-L${LIBC_LIBRARY_DIR}" _sf_idx)
      if(_sf_idx GREATER_EQUAL 0)
        list(REMOVE_AT _iface_libs ${_sf_idx})
        list(INSERT _iface_libs ${_sf_idx} "-L${_hf_libc_dir}")
        set_property(TARGET zephyr_interface PROPERTY INTERFACE_LINK_LIBRARIES "${_iface_libs}")
      endif()
    endif()
  endif()

  # CA32 app sources
  zephyr_sources(soc.c)
  zephyr_library_sources_ifdef(CONFIG_ARM_AARCH32_MMU mmu_regions.c)

  zephyr_linker_sources(RAM_SECTIONS SORT_KEY 0x0 boot_section.ld)

endif()

# ---------------------------------------------------------------------------
# Post-build image packaging
# ---------------------------------------------------------------------------

set(PYTHON_SCRIPT_ARGS
    --soc ${CONFIG_SOC_SERIES}
    --bin-file ${ZEPHYR_BINARY_DIR}/${KERNEL_BIN_NAME}
    --out-dir ${CMAKE_BINARY_DIR}
    --module-dir ${ZEPHYR_HAL_REALTEK_MODULE_DIR}
)

if(CONFIG_BT_COEXIST)
  list(APPEND PYTHON_SCRIPT_ARGS --bt-coexist)
endif()

file(GLOB BIN_FILES "${ZEPHYR_HAL_REALTEK_MODULE_DIR}/zephyr/blobs/ameba/${CONFIG_SOC_SERIES}/bin/*.bin")

if(BIN_FILES)
  # merge_bin.py adds the vendor image2 header. The CA32 run also merges the
  # KM4/KM0 images into the flashable app.bin.
  if(CONFIG_SOC_RTL8730E_KM4)
    list(APPEND PYTHON_SCRIPT_ARGS --core km4)
  elseif(CONFIG_SOC_RTL8730E_KM0)
    list(APPEND PYTHON_SCRIPT_ARGS --core km0)
  endif()

  add_custom_target(raw.map ALL
    DEPENDS ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.raw.map
  )
  add_custom_command(
    OUTPUT ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.raw.map
    COMMAND ${CMAKE_NM} ${ZEPHYR_BINARY_DIR}/${KERNEL_ELF_NAME} | sort > ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.raw.map
    COMMAND ${CMAKE_OBJDUMP} -d ${ZEPHYR_BINARY_DIR}/${KERNEL_ELF_NAME} > ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.asm
    COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_HAL_REALTEK_MODULE_DIR}/ameba/scripts/merge_bin.py ${PYTHON_SCRIPT_ARGS}
    DEPENDS ${ZEPHYR_BINARY_DIR}/${KERNEL_ELF_NAME}
            ${ZEPHYR_HAL_REALTEK_MODULE_DIR}/ameba/${CONFIG_SOC_SERIES}/manifest.json5
  )
else()
  message(WARNING "Missing Realtek blobs. Run: `west blobs fetch hal_realtek`")
endif()
