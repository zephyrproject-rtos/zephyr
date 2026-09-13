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

if(CONFIG_DCACHE_LINE_SIZE)
  # Resolve __DCACHELINE_EXCLUSIVE_NOINIT_SECTION_NAME from sections.h so the
  # CMake generator emits exactly the same section name as the .ld scripts.
  if(NOT DEFINED __DCACHELINE_EXCLUSIVE_NOINIT_SECTION_NAME)
    zephyr_get_include_directories_for_lang(C current_includes)

    # The probe file is preprocessed; the #define value lands on a line we can parse.
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/dcacheline_noinit_probe.c
        "#include <zephyr/linker/sections.h>\n__DCACHELINE_EXCLUSIVE_NOINIT_SECTION_NAME\n")

    execute_process(
      COMMAND ${CMAKE_C_COMPILER} -E -P
              ${current_includes}
              -I${ZEPHYR_BASE}/include
              -I${PROJECT_BINARY_DIR}/include/generated
              -include ${AUTOCONF_H}
              ${CMAKE_CURRENT_BINARY_DIR}/dcacheline_noinit_probe.c
      OUTPUT_VARIABLE probe_out
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET
      RESULT_VARIABLE probe_rc
    )

    if(probe_rc EQUAL 0 AND probe_out MATCHES "^[A-Za-z_][A-Za-z0-9_]*$")
      set(__DCACHELINE_EXCLUSIVE_NOINIT_SECTION_NAME "${probe_out}")
    else()
      message(FATAL_ERROR "Could not extract __DCACHELINE_EXCLUSIVE_NOINIT_SECTION_NAME "
              "from 'zephyr/linker/sections'")
    endif()
  endif()

  zephyr_linker_section_configure(SECTION .noinit
    ALIGN ${CONFIG_DCACHE_LINE_SIZE}
    SYMBOLS __dcacheline_exclusive_noinit_start
  )
  zephyr_linker_section_configure(SECTION .noinit
    KEEP SORT NAME INPUT ".${__DCACHELINE_EXCLUSIVE_NOINIT_SECTION_NAME}*"
  )
  zephyr_linker_section_configure(SECTION .noinit
    ALIGN ${CONFIG_DCACHE_LINE_SIZE}
    SYMBOLS __dcacheline_exclusive_noinit_end
  )
endif()

# TODO: #include <snippets-noinit.ld>
include(${COMMON_ZEPHYR_LINKER_DIR}/kobject-priv-stacks.cmake)
