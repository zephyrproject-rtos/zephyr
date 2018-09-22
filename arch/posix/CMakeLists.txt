zephyr_cc_option_ifdef(CONFIG_LTO -flto)

zephyr_compile_options(
  -fno-freestanding
  -m32
  -MMD
  -MP
  ${ARCH_FLAG}
  -include ${ZEPHYR_BASE}/arch/posix/include/posix_cheats.h
  )

zephyr_compile_options_ifdef(CONFIG_COVERAGE
  -fprofile-arcs
  -ftest-coverage
)
zephyr_link_libraries_ifdef(CONFIG_COVERAGE
	-lgcov
	)

if (CONFIG_ASAN)
	zephyr_compile_options(-fsanitize=address)
	zephyr_link_libraries(-lasan)
	zephyr_ld_options(-fsanitize=address)
endif ()

zephyr_compile_definitions(_POSIX_C_SOURCE=200809 _XOPEN_SOURCE _XOPEN_SOURCE_EXTENDED)

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
