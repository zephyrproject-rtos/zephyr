# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

# Allow passing GCID_REV_CID via -D on the cmake/west command line and forward
# it to the compiler so vg_lite_options.h can use it in an #include path.
if(CONFIG_LV_USE_GPU_VG_LITE AND NOT CONFIG_LV_USE_VG_LITE_DRIVER)
  if(DEFINED GCID_REV_CID)
    # GCID_REV_CID is expected to be something like: gc355/0x0_1216
    zephyr_compile_definitions(GCID_REV_CID=${GCID_REV_CID})
  endif()
endif()

if(CONFIG_LV_USE_GPU_VG_LITE)
  if(NOT CONFIG_LV_USE_VG_LITE_DRIVER)
    zephyr_include_directories(
    ${ZEPHYR_GPU_VGLITE_MODULE_DIR}
    ${ZEPHYR_GPU_VGLITE_MODULE_DIR}/VGLiteKernel/zephyr
    ${ZEPHYR_GPU_VGLITE_MODULE_DIR}/VGLiteKernel
    ${ZEPHYR_GPU_VGLITE_MODULE_DIR}/VGLite
    ${ZEPHYR_GPU_VGLITE_MODULE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${ZEPHYR_BASE}/include
    ${ZEPHYR_BASE}/include/zephyr
  )

    zephyr_library_sources(
    ${ZEPHYR_GPU_VGLITE_MODULE_DIR}/VGLiteKernel/vg_lite_kernel.c
    ${ZEPHYR_GPU_VGLITE_MODULE_DIR}/VGLite/vg_lite.c
    ${ZEPHYR_GPU_VGLITE_MODULE_DIR}/VGLite/vg_lite_image.c
    ${ZEPHYR_GPU_VGLITE_MODULE_DIR}/VGLite/vg_lite_matrix.c
    ${ZEPHYR_GPU_VGLITE_MODULE_DIR}/VGLite/vg_lite_path.c
    ${ZEPHYR_GPU_VGLITE_MODULE_DIR}/VGLite/vg_lite_stroke.c
    ${ZEPHYR_GPU_VGLITE_MODULE_DIR}/VGLite/Series/common/vg_lite_chip.c
    ${ZEPHYR_GPU_VGLITE_MODULE_DIR}/VGLiteKernel/zephyr/vg_lite_hal.c
  )

  endif()
endif()
