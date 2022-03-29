# SPDX-License-Identifier: Apache-2.0

# See root CMakeLists.txt for description and expectations of these macros

macro(toolchain_ld_configure_files)
  configure_file(
       ${ZEPHYR_BASE}/include/zephyr/arch/common/app_data_alignment.ld
       ${PROJECT_BINARY_DIR}/include/generated/app_data_alignment.ld)

  configure_file(
       ${ZEPHYR_BASE}/include/zephyr/linker/app_smem.ld
       ${PROJECT_BINARY_DIR}/include/generated/app_smem.ld)

  configure_file(
       ${ZEPHYR_BASE}/include/zephyr/linker/app_smem_aligned.ld
       ${PROJECT_BINARY_DIR}/include/generated/app_smem_aligned.ld)

  configure_file(
       ${ZEPHYR_BASE}/include/zephyr/linker/app_smem_unaligned.ld
       ${PROJECT_BINARY_DIR}/include/generated/app_smem_unaligned.ld)

  if(CONFIG_LINKER_USE_PINNED_SECTION)
    configure_file(
         ${ZEPHYR_BASE}/include/zephyr/linker/app_smem_pinned.ld
         ${PROJECT_BINARY_DIR}/include/generated/app_smem_pinned.ld)

    configure_file(
         ${ZEPHYR_BASE}/include/zephyr/linker/app_smem_pinned_aligned.ld
         ${PROJECT_BINARY_DIR}/include/generated/app_smem_pinned_aligned.ld)

    configure_file(
         ${ZEPHYR_BASE}/include/zephyr/linker/app_smem_pinned_unaligned.ld
         ${PROJECT_BINARY_DIR}/include/generated/app_smem_pinned_unaligned.ld)
  endif()
endmacro()
