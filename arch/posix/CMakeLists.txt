zephyr_cc_option_ifdef(CONFIG_LTO -flto)

zephyr_ld_option_ifdef(CONFIG_COVERAGE
	-lgcov
	)
zephyr_compile_options(
  -m32
  -MMD -MP -D_POSIX_C_SOURCE=199309
  ${TOOLCHAIN_C_FLAGS}
  ${ARCH_FLAG}
  -include ${PROJECT_SOURCE_DIR}/arch/posix/include/posix_cheats.h
  )

add_subdirectory(soc)
add_subdirectory(core)
