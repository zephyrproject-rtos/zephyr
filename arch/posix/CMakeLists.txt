zephyr_cc_option_ifdef(CONFIG_LTO -flto)

zephyr_compile_options(
  -Wno-format-truncation
  -fno-freestanding
  -Wno-undef
  -Wno-implicit-function-declaration
  -m32
  -MMD
  -MP
  ${ARCH_FLAG}
  -include ${PROJECT_SOURCE_DIR}/arch/posix/include/posix_cheats.h
  )

zephyr_compile_options_ifdef(CONFIG_COVERAGE
  -fprofile-arcs
  -ftest-coverage
)
zephyr_link_libraries_ifdef(CONFIG_COVERAGE
	-lgcov
	)

zephyr_compile_definitions(_POSIX_C_SOURCE=200809)

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


add_subdirectory(soc)
add_subdirectory(core)
