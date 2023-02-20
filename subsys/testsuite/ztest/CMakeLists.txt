# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(
  ${ZEPHYR_BASE}/subsys/testsuite/include
  ${ZEPHYR_BASE}/subsys/testsuite/ztest/include
  )

if(DEFINED TC_RUNID)
  zephyr_compile_definitions(TC_RUNID=${TC_RUNID})
endif()

zephyr_library()
zephyr_library_sources_ifndef(CONFIG_ZTEST_NEW_API src/ztest.c)
zephyr_library_sources_ifdef(CONFIG_ZTEST_NEW_API  src/ztest_new.c)
zephyr_library_sources(                            src/ztest_error_hook.c)
zephyr_library_sources_ifdef(CONFIG_ZTEST_NEW_API  src/ztest_rules.c)
zephyr_library_sources_ifdef(CONFIG_ZTEST_MOCKING  src/ztest_mock.c)
zephyr_library_sources_ifdef(CONFIG_ZTRESS         src/ztress.c)


if(CONFIG_ARCH_POSIX)
  zephyr_library_sources_ifdef(CONFIG_ZTEST_NEW_API src/ztest_posix.c)
else()
  zephyr_library_sources_ifdef(CONFIG_ZTEST_NEW_API src/ztest_defaults.c)
endif()
