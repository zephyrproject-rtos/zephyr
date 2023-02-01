# The coverage linker flag is specific for clang.
if (NOT CONFIG_COVERAGE_GCOV)
  set_property(TARGET linker PROPERTY coverage --coverage)
endif()

if (CONFIG_USERSPACE)
  check_set_linker_property(TARGET linker PROPERTY no_global_merge -mno-global-merge)
endif()
