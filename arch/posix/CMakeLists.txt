# SPDX-License-Identifier: Apache-2.0

zephyr_compile_options(
  -m32
  -MMD
  -MP
  ${ARCH_FLAG}
  -include ${ZEPHYR_BASE}/arch/posix/include/posix_cheats.h
  )

# @Intent: Obtain compiler specific flags for no freestanding compilation
toolchain_cc_no_freestanding_options()

zephyr_include_directories(${BOARD_DIR})

zephyr_compile_options_ifdef(CONFIG_COVERAGE
  -fprofile-arcs
  -ftest-coverage
  -fno-inline
)
zephyr_link_libraries_ifdef(CONFIG_COVERAGE
	-lgcov
	)

if (CONFIG_ASAN)
	zephyr_compile_options(-fsanitize=address)
	zephyr_link_libraries(-lasan)
	zephyr_ld_options(-fsanitize=address)
endif ()

zephyr_compile_definitions(_POSIX_C_SOURCE=200809 _XOPEN_SOURCE=600 _XOPEN_SOURCE_EXTENDED)

zephyr_ld_options(
  -ldl
  -pthread
  -m32
)

# About the -include directive: The reason to do it this way, is because in this
# manner it is transparent to the application. Otherwise posix_cheats.h needs to
# be included in all the applications' files which define main( ), and in any
# app file which uses the pthreads like API provided by Zephyr
# ( include/posix/pthread.h / kernel/pthread.c ) [And any future API added to
# Zephyr which will clash with the native POSIX API] . It would also need to
# be included in a few zephyr kernel files.


add_subdirectory(core)
