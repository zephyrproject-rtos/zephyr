# The coverage linker flag is specific for clang.
if (NOT CONFIG_COVERAGE_GCOV)
  set_property(TARGET linker PROPERTY coverage --coverage)
endif()
