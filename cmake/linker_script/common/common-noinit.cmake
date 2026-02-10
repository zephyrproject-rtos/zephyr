# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
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

if(CONFIG_DCACHE_LINE_SIZE AND CONFIG_DCACHE_LINE_SIZE>0)
  zephyr_linker_section(NAME ".${__DCACHELINE_EXCLUSIVE_NOINIT_SECTION_NAME}" GROUP NOINIT_REGION
    SUBALIGN $(CONFIG_DCACHE_LINE_SIZE) NOINPUT
  )
  zephyr_linker_section_configure(SECTION ${__DCACHELINE_EXCLUSIVE_NOINIT_SECTION_NAME}
    KEEP SORT NAME INPUT ".${__DCACHELINE_EXCLUSIVE_NOINIT_SECTION_NAME}.*"
    ALIGN $(CONFIG_DCACHE_LINE_SIZE)
    SYMBOLS __dcacheline_exclusive_noinit_start __dcacheline_exclusive_noinit_end
  )
endif()

# TODO: #include <snippets-noinit.ld>
include(${COMMON_ZEPHYR_LINKER_DIR}/kobject-priv-stacks.cmake)
