# SPDX-License-Identifier: Apache-2.0
# The contents of this file is based on include/zephyr/linker/common-noinit.ld
# Please keep in sync

zephyr_linker_section(NAME .noinit GROUP NOINIT_REGION TYPE NOLOAD NOINIT)

if(CONFIG_USERSPACE)
  zephyr_linker_section_configure(
    SECTION .noinit
    INPUT ".user_stacks*"
    SYMBOLS z_user_stacks_start z_user_stacks_end)

endif()

zephyr_linker_section_configure(
  SECTION .noinit
  INPUT ".kernel_stacks*"
  SYMBOLS z_kernel_stacks_start z_kernel_stacks_end)

# TODO: #include <snippets-noinit.ld>
include(${COMMON_ZEPHYR_LINKER_DIR}/kobject-priv-stacks.cmake)
