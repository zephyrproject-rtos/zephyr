# Copyright (c) 2019 jan.van_winkel@dxplore.eu
# Copyright (c) 2020 Teslabs Engineering S.L.
# Copyright (c) 2023 Fabian Blatz <fabianblatz@gmail.com>
# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_LVGL)

set(ZEPHYR_CURRENT_LIBRARY lvgl)
set(LVGL_DIR ${ZEPHYR_LVGL_MODULE_DIR})

zephyr_interface_library_named(LVGL)
zephyr_library()

zephyr_include_directories(${LVGL_DIR}/src/)
zephyr_include_directories(include)

zephyr_compile_definitions(LV_CONF_INCLUDE_SIMPLE=1)
zephyr_library_compile_definitions(_POSIX_C_SOURCE=200809L)
zephyr_compile_definitions(LV_CONF_PATH="${CMAKE_CURRENT_SOURCE_DIR}/include/lv_conf.h")

# Allow passing GCID_REV_CID via -D on the cmake/west command line and forward
# it to the compiler so vg_lite_options.h can use it in an #include path.
  if(CONFIG_LV_USE_GPU_VG_LITE AND NOT CONFIG_LV_USE_VG_LITE_DRIVER)
    if(DEFINED GCID_REV_CID)
      # GCID_REV_CID is expected to be something like: gc355/0x0_1216
      zephyr_compile_definitions(GCID_REV_CID=${GCID_REV_CID})
    endif()
  endif()

file(GLOB_RECURSE LVGL_SRC_FILES CONFIGURE_DEPENDS
    ${LVGL_DIR}/src/*.c
)

set(LVGL_EXCLUDE_PATTERNS
    "${LVGL_DIR}/src/osal/lv_cmsis_rtos2.c"
    "${LVGL_DIR}/src/osal/lv_freertos.c"
    "${LVGL_DIR}/src/osal/lv_linux.c"
    "${LVGL_DIR}/src/osal/lv_mqx.c"
    "${LVGL_DIR}/src/osal/lv_os_none.c"
    "${LVGL_DIR}/src/osal/lv_pthread.c"
    "${LVGL_DIR}/src/osal/lv_rtthread.c"
    "${LVGL_DIR}/src/osal/lv_sdl2.c"
    "${LVGL_DIR}/src/osal/lv_windows.c"

    "${LVGL_DIR}/src/drivers/*"

    # Remove libs/gltf since this leads to build warnings
    "${LVGL_DIR}/src/libs/gltf/*"

    "${LVGL_DIR}/src/stdlib/builtin/lv_mem_core_builtin.c"
    "${LVGL_DIR}/src/stdlib/builtin/lv_sprintf_builtin.c"
    "${LVGL_DIR}/src/stdlib/builtin/lv_string_builtin.c"
    "${LVGL_DIR}/src/stdlib/clib/lv_mem_core_clib.c"

    "${LVGL_DIR}/src/stdlib/rtthread/*"
    "${LVGL_DIR}/src/stdlib/micropython/*"
    "${LVGL_DIR}/src/stdlib/uefi/*"

    "${LVGL_DIR}/src/debugging/test/*"
)

  if(CONFIG_LV_USE_GPU_VG_LITE)
    if(CONFIG_LV_USE_VG_LITE_DRIVER)
    else()
      # Exclude vg_lite_driver from LVGL source tree
      list(APPEND LVGL_EXCLUDE_PATTERNS "${LVGL_DIR}/src/libs/vg_lite_driver/*")
    endif()
  endif()

foreach(pattern IN LISTS LVGL_EXCLUDE_PATTERNS)
    file(GLOB_RECURSE LVGL_EXCLUDED CONFIGURE_DEPENDS ${pattern})
    list(REMOVE_ITEM LVGL_SRC_FILES ${LVGL_EXCLUDED})
endforeach()

zephyr_library_sources(${LVGL_SRC_FILES})

zephyr_library_sources(
    lvgl.c
    lvgl_display.c
    lvgl_display_mono.c
    lvgl_display_8bit.c
    lvgl_display_16bit.c
    lvgl_display_24bit.c
    lvgl_display_32bit.c
    lvgl_display_mono.c
    lvgl_zephyr_osal.c
)

  if(CONFIG_LV_USE_GPU_VG_LITE)
    if(CONFIG_LV_USE_VG_LITE_DRIVER)
    else()
      zephyr_include_directories(
      ${ZEPHYR_BASE}/../modules/gpu-vglite
      ${ZEPHYR_BASE}/../modules/gpu-vglite/VGLiteKernel/zephyr
      ${ZEPHYR_BASE}/../modules/gpu-vglite/VGLiteKernel
      ${ZEPHYR_BASE}/../modules/gpu-vglite/VGLite
      ${ZEPHYR_BASE}/../modules/gpu-vglite/inc
      ${CMAKE_CURRENT_SOURCE_DIR}/include
      ${ZEPHYR_BASE}/include
      ${ZEPHYR_BASE}/include/zephyr
    )

      zephyr_library_sources(
      ${ZEPHYR_BASE}/../modules/gpu-vglite/VGLiteKernel/vg_lite_kernel.c
      ${ZEPHYR_BASE}/../modules/gpu-vglite/VGLite/vg_lite.c
      ${ZEPHYR_BASE}/../modules/gpu-vglite/VGLite/vg_lite_image.c
      ${ZEPHYR_BASE}/../modules/gpu-vglite/VGLite/vg_lite_matrix.c
      ${ZEPHYR_BASE}/../modules/gpu-vglite/VGLite/vg_lite_path.c
      ${ZEPHYR_BASE}/../modules/gpu-vglite/VGLite/vg_lite_stroke.c
      ${ZEPHYR_BASE}/../modules/gpu-vglite/VGLite/Series/common/vg_lite_chip.c
      ${ZEPHYR_BASE}/../modules/gpu-vglite/VGLiteKernel/zephyr/vg_lite_hal.c
    )

    endif()
  endif()

zephyr_library_sources_ifdef(CONFIG_LV_Z_USE_FILESYSTEM lvgl_fs.c)
zephyr_library_sources_ifdef(CONFIG_LV_Z_MEM_POOL_SYS_HEAP lvgl_mem.c)
zephyr_library_sources_ifdef(CONFIG_LV_Z_SHELL lvgl_shell.c)

zephyr_library_sources(input/lvgl_common_input.c)
zephyr_library_sources_ifdef(CONFIG_LV_Z_POINTER_INPUT input/lvgl_pointer_input.c)
zephyr_library_sources_ifdef(CONFIG_LV_Z_BUTTON_INPUT input/lvgl_button_input.c)
zephyr_library_sources_ifdef(CONFIG_LV_Z_ENCODER_INPUT input/lvgl_encoder_input.c)
zephyr_library_sources_ifdef(CONFIG_LV_Z_KEYPAD_INPUT input/lvgl_keypad_input.c)

zephyr_library_link_libraries(LVGL)
target_link_libraries(LVGL INTERFACE zephyr_interface)

endif()
